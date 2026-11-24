#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later

"""Left-to-right grid auto-arrangement for PatchCanvas nodes."""

from collections import defaultdict, deque
from dataclasses import dataclass, field
from itertools import permutations
from statistics import median


ROLE_SOURCE = 0  # Output-only nodes (far left)
ROLE_MIDDLE = 1  # Nodes with both inputs and outputs, or no ports (middle columns)
ROLE_SINK = 2    # Input-only nodes (far right)

BASE_H_GAP = 120.0
MIN_H_GAP = 64.0
MAX_H_GAP = 240.0
BASE_V_GAP = 28.0
MIN_V_GAP = 14.0
BASE_COMP_GAP = 48.0
MIN_COMP_GAP = 24.0
CANVAS_MARGIN = 40.0
CLEARANCE_PAD_X = 6.0
CLEARANCE_PAD_Y = 12.0


@dataclass(frozen=True)
class LayoutNode:
    key: object
    name: str
    width: float
    height: float
    has_inputs: bool
    has_outputs: bool
    in_port_y: dict = field(default_factory=dict)
    out_port_y: dict = field(default_factory=dict)

    @property
    def role(self):
        if self.has_outputs and not self.has_inputs:
            return ROLE_SOURCE
        if self.has_inputs and not self.has_outputs:
            return ROLE_SINK
        return ROLE_MIDDLE


@dataclass(frozen=True)
class LayoutEdge:
    src_key: object
    src_port_id: object
    dst_key: object
    dst_port_id: object


@dataclass
class _ColumnItem:
    key: object
    height: float
    is_dummy: bool = False
    sort_name: str = ""
    # For dummy nodes, store the original bundle endpoints
    bundle: tuple = ()


@dataclass(frozen=True)
class _Segment:
    src_item: object
    dst_item: object
    src_offset: float
    dst_offset: float
    weight: int = 1


def _break_cycles_greedy(node_keys, weighted_edges, sort_key, left_bias=None, right_bias=None):
    """Return set of (u, v) edges to reverse so the directed graph becomes acyclic.

    Uses the Eades-Lin-Smyth greedy heuristic: peel sources/sinks, otherwise pick
    the node maximizing (out_weight - in_weight + source/sink bias). Leaves DAGs untouched.
    """
    left_bias = left_bias or {}
    right_bias = right_bias or {}
    remaining = set(node_keys)
    adj_out = {k: {} for k in remaining}
    adj_in = {k: {} for k in remaining}
    for (u, v), w in weighted_edges.items():
        if u in remaining and v in remaining and u != v:
            adj_out[u][v] = w
            adj_in[v][u] = w

    left = []
    right = []
    while remaining:
        # Peel nodes with no remaining incoming edges
        progressed = True
        while progressed:
            progressed = False
            sources = sorted((k for k in remaining if not adj_in[k]), key=sort_key)
            for u in sources:
                left.append(u)
                remaining.remove(u)
                for v in list(adj_out[u]):
                    adj_in[v].pop(u, None)
                adj_out[u].clear()
                progressed = True

            sinks = sorted((k for k in remaining if not adj_out[k]), key=sort_key)
            for v in sinks:
                right.append(v)
                remaining.remove(v)
                for u in list(adj_in[v]):
                    adj_out[u].pop(v, None)
                adj_in[v].clear()
                progressed = True

        if not remaining:
            break

        # Cycle exists: pick vertex with strongest left-side pull (more outgoing, fed by sources,
        # not feeding sinks, earlier name on ties)
        best = min(
            remaining,
            key=lambda k: (
                -(sum(adj_out[k].values()) - sum(adj_in[k].values())
                  + 2 * left_bias.get(k, 0) - 2 * right_bias.get(k, 0)),
                len(adj_in[k]),
                sort_key(k),
            ),
        )
        left.append(best)
        remaining.remove(best)
        for v in list(adj_out[best]):
            adj_in[v].pop(best, None)
        for u in list(adj_in[best]):
            adj_out[u].pop(best, None)
        adj_out[best].clear()
        adj_in[best].clear()

    order = left + list(reversed(right))
    rank = {k: idx for idx, k in enumerate(order)}
    return {(u, v) for (u, v) in weighted_edges if u in rank and v in rank and rank[u] > rank[v]}


def _assign_middle_layers(middle_keys, edges_all, nodes_by_key):
    """Assign each middle node a 0-based layer index via longest-path on the DAG."""
    middle_set = set(middle_keys)
    if not middle_set:
        return {}

    weighted_mid = defaultdict(int)
    source_in_w = defaultdict(int)
    sink_out_w = defaultdict(int)
    for e in edges_all:
        if e.src_key == e.dst_key:
            continue
        if e.src_key in middle_set and e.dst_key in middle_set:
            weighted_mid[(e.src_key, e.dst_key)] += 1
        elif e.dst_key in middle_set and nodes_by_key[e.src_key].role == ROLE_SOURCE:
            source_in_w[e.dst_key] += 1
        elif e.src_key in middle_set and nodes_by_key[e.dst_key].role == ROLE_SINK:
            sink_out_w[e.src_key] += 1

    sort_key = lambda k: (nodes_by_key[k].name.lower(), str(k))
    reversed_edges = _break_cycles_greedy(
        middle_set, weighted_mid, sort_key, left_bias=source_in_w, right_bias=sink_out_w
    )

    dag_out = {k: [] for k in middle_set}
    dag_in = {k: [] for k in middle_set}
    indeg = {k: 0 for k in middle_set}
    for (u, v) in weighted_mid:
        if (u, v) in reversed_edges:
            u, v = v, u
        if v not in dag_out[u]:
            dag_out[u].append(v)
            dag_in[v].append(u)
            indeg[v] += 1

    queue = deque(sorted((k for k, d in indeg.items() if d == 0), key=sort_key))
    topo = []
    while queue:
        u = queue.popleft()
        topo.append(u)
        for v in dag_out[u]:
            indeg[v] -= 1
            if indeg[v] == 0:
                queue.append(v)

    layers = {k: 0 for k in middle_set}
    for u in topo:
        for v in dag_out[u]:
            if layers[v] < layers[u] + 1:
                layers[v] = layers[u] + 1

    # Tighten middle nodes that have no incoming edges (neither source nor middle)
    # closer to the downstream middle nodes they feed, reducing long skip-edges.
    for u in reversed(topo):
        if not dag_in[u] and not source_in_w[u] and dag_out[u]:
            layers[u] = min(layers[v] - 1 for v in dag_out[u])

    return layers


def _pack_column_pav(desired, heights, v_gap):
    """Find y[i] minimizing sum((y[i] - desired[i])**2) s.t. y[i+1] >= y[i] + heights[i] + v_gap.

    Exact O(n) Pool Adjacent Violators algorithm for isotonic regression with offsets.
    """
    n = len(desired)
    if n == 0:
        return []

    # Transform: let offset[0] = 0, offset[i] = sum_{j<i} (heights[j] + v_gap).
    # Then z[i] = y[i] - offset[i] must be non-decreasing: z[0] <= z[1] <= ... <= z[n-1].
    offsets = [0.0] * n
    for i in range(1, n):
        offsets[i] = offsets[i - 1] + heights[i - 1] + v_gap

    # Each block is [start, end, sum_val, count]
    blocks = []
    for i in range(n):
        val = desired[i] - offsets[i]
        blocks.append([i, i, val, 1])
        while len(blocks) >= 2:
            prev = blocks[-2]
            curr = blocks[-1]
            if prev[2] / prev[3] > curr[2] / curr[3]:
                prev[1] = curr[1]
                prev[2] += curr[2]
                prev[3] += curr[3]
                blocks.pop()
            else:
                break

    y = [0.0] * n
    for start, end, total, count in blocks:
        mean_z = total / count
        for i in range(start, end + 1):
            y[i] = mean_z + offsets[i]
    return y


def _count_pair_crossings(left_order, right_order, segments, items_by_key, v_gap):
    """Count port-aware crossings and total vertical cable span between two adjacent columns."""
    if not segments:
        return 0, 0.0

    left_y = {}
    cursor = 0.0
    for k in left_order:
        left_y[k] = cursor
        cursor += items_by_key[k].height + v_gap

    right_y = {}
    cursor = 0.0
    for k in right_order:
        right_y[k] = cursor
        cursor += items_by_key[k].height + v_gap

    # Build list of (y_left, y_right, weight)
    coords = []
    total_span = 0.0
    for seg in segments:
        yl = left_y[seg.src_item] + seg.src_offset
        yr = right_y[seg.dst_item] + seg.dst_offset
        coords.append((yl, yr, seg.weight))
        total_span += abs(yl - yr) * seg.weight

    crossings = 0
    m = len(coords)
    for i in range(m):
        yl1, yr1, w1 = coords[i]
        for j in range(i + 1, m):
            yl2, yr2, w2 = coords[j]
            if (yl1 - yl2) * (yr1 - yr2) < -1e-6:
                crossings += w1 * w2

    return crossings, total_span


def _total_crossings(orders, segs_by_gap, items_by_key, v_gap):
    cross = 0
    span = 0.0
    for c, segs in segs_by_gap.items():
        if segs:
            cr, sp = _count_pair_crossings(orders[c], orders[c + 1], segs, items_by_key, v_gap)
            cross += cr
            span += sp
    return cross, span


def _minimize_crossings(orders, segs_by_gap, items_by_key, v_gap):
    """Port-aware Sugiyama crossing minimization with barycenter sweeps, exact small-column search, and transpose."""
    cols = sorted(orders.keys())
    if len(cols) <= 1:
        return orders

    in_segs = defaultdict(list)
    out_segs = defaultdict(list)
    for segs in segs_by_gap.values():
        for seg in segs:
            out_segs[seg.src_item].append(seg)
            in_segs[seg.dst_item].append(seg)

    def col_y_map(order):
        ymap = {}
        cursor = 0.0
        for k in order:
            ymap[k] = cursor
            cursor += items_by_key[k].height + v_gap
        return ymap

    def column_local_cost(c, cand_order):
        cross = 0
        span = 0.0
        if c - 1 in segs_by_gap and segs_by_gap[c - 1]:
            cr, sp = _count_pair_crossings(orders[c - 1], cand_order, segs_by_gap[c - 1], items_by_key, v_gap)
            cross += cr
            span += sp
        if c in segs_by_gap and segs_by_gap[c]:
            cr, sp = _count_pair_crossings(cand_order, orders[c + 1], segs_by_gap[c], items_by_key, v_gap)
            cross += cr
            span += sp
        return cross, span

    def optimize_column_exact_or_transpose(c):
        order = orders[c]
        if len(order) <= 1:
            return
        if len(order) <= 6:
            best_order = order
            best_cost = column_local_cost(c, order)
            for perm in permutations(order):
                cand = list(perm)
                cost = column_local_cost(c, cand)
                if cost[0] < best_cost[0] or (cost[0] == best_cost[0] and cost[1] + 1e-6 < best_cost[1]):
                    best_cost = cost
                    best_order = cand
            orders[c] = best_order
            return

        improved = True
        while improved:
            improved = False
            best_cost = column_local_cost(c, orders[c])
            for i in range(len(orders[c]) - 1):
                orders[c][i], orders[c][i + 1] = orders[c][i + 1], orders[c][i]
                cand_cost = column_local_cost(c, orders[c])
                if cand_cost[0] < best_cost[0] or (cand_cost[0] == best_cost[0] and cand_cost[1] + 1e-6 < best_cost[1]):
                    best_cost = cand_cost
                    improved = True
                else:
                    orders[c][i], orders[c][i + 1] = orders[c][i + 1], orders[c][i]

    best_orders = {c: list(orders[c]) for c in cols}
    best_score = _total_crossings(orders, segs_by_gap, items_by_key, v_gap)

    def maybe_record():
        nonlocal best_orders, best_score
        score = _total_crossings(orders, segs_by_gap, items_by_key, v_gap)
        if score[0] < best_score[0] or (score[0] == best_score[0] and score[1] + 1e-6 < best_score[1]):
            best_score = score
            best_orders = {c: list(orders[c]) for c in cols}
        return score

    for _ in range(10):
        prev_best = best_score

        # Full left-to-right barycenter sweep (propagates multi-column dummy chains cleanly)
        for c in cols[1:]:
            if len(orders[c]) <= 1:
                continue
            prev_y = col_y_map(orders[c - 1])
            curr_idx = {k: idx for idx, k in enumerate(orders[c])}

            def bary_left(item_key):
                segs = in_segs.get(item_key)
                if not segs:
                    return (curr_idx[item_key] * 100.0, items_by_key[item_key].sort_name)
                total_w = sum(s.weight for s in segs)
                avg_y = sum((prev_y[s.src_item] + s.src_offset) * s.weight for s in segs) / total_w
                return (avg_y, items_by_key[item_key].sort_name)

            orders[c].sort(key=bary_left)
        maybe_record()

        # Local exact / transpose polish after full L->R propagation
        for c in cols:
            optimize_column_exact_or_transpose(c)
        maybe_record()

        # Full right-to-left barycenter sweep
        for c in reversed(cols[:-1]):
            if len(orders[c]) <= 1:
                continue
            next_y = col_y_map(orders[c + 1])
            curr_idx = {k: idx for idx, k in enumerate(orders[c])}

            def bary_right(item_key):
                segs = out_segs.get(item_key)
                if not segs:
                    return (curr_idx[item_key] * 100.0, items_by_key[item_key].sort_name)
                total_w = sum(s.weight for s in segs)
                avg_y = sum((next_y[s.dst_item] + s.dst_offset) * s.weight for s in segs) / total_w
                return (avg_y, items_by_key[item_key].sort_name)

            orders[c].sort(key=bary_right)
        maybe_record()

        # Local exact / transpose polish after full R->L propagation
        for c in reversed(cols):
            optimize_column_exact_or_transpose(c)
        score = maybe_record()

        if score[0] == 0 and best_score == prev_best:
            break

    return best_orders


def _bezier_x(t, x1, x2):
    """Evaluate x(t) for PatchCanvas cubic Bezier with horizontal tangents at midpoint."""
    mx = 0.5 * (x1 + x2)
    u = 1.0 - t
    return u * u * u * x1 + 3.0 * u * u * t * mx + 3.0 * u * t * t * mx + t * t * t * x2


def _bezier_t_for_x(target_x, x1, x2):
    """Binary search t in [0, 1] where _bezier_x(t, x1, x2) == target_x (assumes x1 < x2)."""
    if target_x <= x1:
        return 0.0
    if target_x >= x2:
        return 1.0
    lo, hi = 0.0, 1.0
    for _ in range(16):
        mid = 0.5 * (lo + hi)
        if _bezier_x(mid, x1, x2) < target_x:
            lo = mid
        else:
            hi = mid
    return 0.5 * (lo + hi)


def edge_y_range_over_x(x1, y1, x2, y2, bx_left, bx_right, use_bezier=True):
    """Return (y_min, y_max) of a forward connection curve over [bx_left, bx_right], or None if disjoint."""
    if x2 <= x1 or bx_right <= x1 or bx_left >= x2:
        return None
    xa = max(x1, bx_left)
    xb = min(x2, bx_right)
    if xb <= xa:
        return None

    if use_bezier:
        ta = _bezier_t_for_x(xa, x1, x2)
        tb = _bezier_t_for_x(xb, x1, x2)
        ya = y1 + (y2 - y1) * (3.0 * ta * ta - 2.0 * ta * ta * ta)
        yb = y1 + (y2 - y1) * (3.0 * tb * tb - 2.0 * tb * tb * tb)
    else:
        dx = x2 - x1
        ya = y1 + (y2 - y1) * ((xa - x1) / dx)
        yb = y1 + (y2 - y1) * ((xb - x1) / dx)

    return (ya, yb) if ya <= yb else (yb, ya)


def _assign_component_y(orders, segs_by_gap, items_by_key, v_gap):
    """Assign Y coordinates to all items (real boxes + dummy corridors) in a connected component."""
    cols = sorted(orders.keys())
    in_segs = defaultdict(list)
    out_segs = defaultdict(list)
    for segs in segs_by_gap.values():
        for seg in segs:
            out_segs[seg.src_item].append(seg)
            in_segs[seg.dst_item].append(seg)

    # Initial top-down packing
    y_pos = {}
    for c in cols:
        cursor = 0.0
        for k in orders[c]:
            y_pos[k] = cursor
            cursor += items_by_key[k].height + v_gap

    def relax_column(c, use_left, use_right):
        order = orders[c]
        if not order:
            return
        desired = []
        heights = [items_by_key[k].height for k in order]
        for k in order:
            targets = []
            if use_left:
                for seg in in_segs.get(k, ()):
                    ty = y_pos[seg.src_item] + seg.src_offset - seg.dst_offset
                    targets.extend([ty] * seg.weight)
            if use_right:
                for seg in out_segs.get(k, ()):
                    ty = y_pos[seg.dst_item] + seg.dst_offset - seg.src_offset
                    targets.extend([ty] * seg.weight)
            desired.append(median(targets) if targets else y_pos[k])

        packed = _pack_column_pav(desired, heights, v_gap)
        for k, yv in zip(order, packed):
            y_pos[k] = yv

    # Alternating alignment sweeps
    for _ in range(4):
        for c in cols[1:]:
            relax_column(c, True, False)
        for c in reversed(cols[:-1]):
            relax_column(c, False, True)

    # Final two-sided balancing sweeps
    for _ in range(4):
        for c in cols:
            relax_column(c, True, True)

    return y_pos


def _clear_edge_box_overlaps(comp_nodes, comp_edges, col_of, orders, x_of, y_pos, nodes_by_key, v_gap, use_bezier):
    """Push intermediate boxes vertically clear of any forward edge curves that intersect them."""
    forward_edges = [
        e for e in comp_edges
        if e.src_key in comp_nodes and e.dst_key in comp_nodes
        and col_of[e.dst_key] - col_of[e.src_key] > 1
    ]
    if not forward_edges:
        return y_pos

    # Map each real box's rank in its column and dummy node ranks for preferred push direction
    rank_in_col = {}
    for c, order in orders.items():
        for idx, item_key in enumerate(order):
            rank_in_col[item_key] = idx

    real_order_by_col = {
        c: [k for k in order if k in comp_nodes]
        for c, order in orders.items()
    }

    def count_collisions(cur_y):
        hits = []
        for e in forward_edges:
            sn = nodes_by_key[e.src_key]
            dn = nodes_by_key[e.dst_key]
            x1 = x_of[e.src_key] + sn.width
            y1 = cur_y[e.src_key] + sn.out_port_y.get(e.src_port_id, sn.height * 0.5)
            x2 = x_of[e.dst_key]
            y2 = cur_y[e.dst_key] + dn.in_port_y.get(e.dst_port_id, dn.height * 0.5)
            for c in range(col_of[e.src_key] + 1, col_of[e.dst_key]):
                for bk in real_order_by_col.get(c, ()):
                    bn = nodes_by_key[bk]
                    bx_l = x_of[bk] - CLEARANCE_PAD_X
                    bx_r = x_of[bk] + bn.width + CLEARANCE_PAD_X
                    yrange = edge_y_range_over_x(x1, y1, x2, y2, bx_l, bx_r, use_bezier)
                    if yrange is None:
                        continue
                    ey_min, ey_max = yrange
                    by_top = cur_y[bk] - CLEARANCE_PAD_Y
                    by_bot = cur_y[bk] + bn.height + CLEARANCE_PAD_Y
                    if ey_max >= by_top and ey_min <= by_bot:
                        hits.append((c, bk, e, ey_min, ey_max))
        return hits

    best_y = dict(y_pos)
    best_hits = len(count_collisions(best_y))
    if best_hits == 0:
        return best_y

    cur_y = dict(y_pos)
    for _ in range(12):
        hits = count_collisions(cur_y)
        if not hits:
            return cur_y

        # Group required vertical nudges per intermediate column
        col_nudges = defaultdict(list)
        for c, bk, e, ey_min, ey_max in hits:
            bn = nodes_by_key[bk]
            dummy_key = ("__dummy__", e.src_key, e.dst_key, c)
            # Choose push direction matching the dummy-corridor ordering when available
            if dummy_key in rank_in_col and bk in rank_in_col:
                push_down = rank_in_col[bk] > rank_in_col[dummy_key]
            else:
                box_mid = cur_y[bk] + bn.height * 0.5
                edge_mid = 0.5 * (ey_min + ey_max)
                push_down = box_mid >= edge_mid

            if push_down:
                target = ey_max + CLEARANCE_PAD_Y + 2.0
            else:
                target = ey_min - CLEARANCE_PAD_Y - bn.height - 2.0
            col_nudges[c].append((bk, push_down, target))

        for c, nudges in col_nudges.items():
            real_col = real_order_by_col.get(c, ())
            if not real_col:
                continue
            desired = {bk: cur_y[bk] for bk in real_col}
            for bk, push_down, target in nudges:
                if push_down:
                    desired[bk] = max(desired[bk], target)
                else:
                    desired[bk] = min(desired[bk], target)
            heights = [nodes_by_key[bk].height for bk in real_col]
            packed = _pack_column_pav([desired[bk] for bk in real_col], heights, v_gap)
            for bk, yv in zip(real_col, packed):
                cur_y[bk] = yv

        new_hits = len(count_collisions(cur_y))
        if new_hits < best_hits:
            best_hits = new_hits
            best_y = dict(cur_y)
        if new_hits == 0:
            return best_y

    return best_y


def _layout_with_spacing(nodes, edges, h_gap_base, v_gap, comp_gap, use_bezier):
    """Compute relative (x, y) positions for all nodes given spacing parameters."""
    nodes_by_key = {n.key: n for n in nodes}
    valid_edges = [
        e for e in edges
        if e.src_key in nodes_by_key and e.dst_key in nodes_by_key and e.src_key != e.dst_key
    ]

    # Build undirected adjacency to find connected components
    undir = {n.key: set() for n in nodes}
    for e in valid_edges:
        undir[e.src_key].add(e.dst_key)
        undir[e.dst_key].add(e.src_key)

    visited = set()
    components = []
    isolated = []
    for n in sorted(nodes, key=lambda item: (item.role, item.name.lower(), str(item.key))):
        if n.key in visited:
            continue
        if not undir[n.key]:
            visited.add(n.key)
            isolated.append(n.key)
            continue
        comp = set()
        queue = deque([n.key])
        visited.add(n.key)
        while queue:
            u = queue.popleft()
            comp.add(u)
            for v in undir[u]:
                if v not in visited:
                    visited.add(v)
                    queue.append(v)
        components.append(comp)

    # Sort connected components: largest first, then alphabetically by earliest node name
    components.sort(
        key=lambda comp: (-len(comp), min(nodes_by_key[k].name.lower() for k in comp))
    )

    # Global role presence determines column structure so all sources share col 0
    # and all sinks share the rightmost column.
    has_sources = any(n.role == ROLE_SOURCE for n in nodes)
    has_middle = any(n.role == ROLE_MIDDLE for n in nodes)
    has_sinks = any(n.role == ROLE_SINK for n in nodes)

    middle_offset = 1 if has_sources else 0

    # Assign middle layers per component (and layer 0 for isolated middle nodes)
    mid_layers = {}
    for comp in components:
        comp_mid = [k for k in comp if nodes_by_key[k].role == ROLE_MIDDLE]
        comp_edges = [e for e in valid_edges if e.src_key in comp and e.dst_key in comp]
        mid_layers.update(_assign_middle_layers(comp_mid, comp_edges, nodes_by_key))
    for k in isolated:
        if nodes_by_key[k].role == ROLE_MIDDLE:
            mid_layers[k] = 0

    max_mid_layer = max(mid_layers.values(), default=0)
    num_mid_cols = (max_mid_layer + 1) if has_middle else 0
    sink_col = middle_offset + num_mid_cols
    num_cols = sink_col + (1 if has_sinks else 0)

    col_of = {}
    for n in nodes:
        if n.role == ROLE_SOURCE:
            col_of[n.key] = 0
        elif n.role == ROLE_SINK:
            col_of[n.key] = sink_col
        else:
            col_of[n.key] = middle_offset + mid_layers.get(n.key, 0)

    # Compute column widths and horizontal gaps
    col_widths = [0.0] * num_cols
    for n in nodes:
        c = col_of[n.key]
        if n.width > col_widths[c]:
            col_widths[c] = n.width

    # Count distinct connections crossing each column gap to widen busy channels gently
    gap_traffic = [0] * max(0, num_cols - 1)
    for e in valid_edges:
        c1, c2 = col_of[e.src_key], col_of[e.dst_key]
        lo, hi = (c1, c2) if c1 < c2 else (c2, c1)
        for c in range(lo, hi):
            gap_traffic[c] += 1

    col_x = [0.0] * num_cols
    for c in range(1, num_cols):
        extra = max(0, gap_traffic[c - 1] - 4) * 6.0
        gap = min(MAX_H_GAP, h_gap_base + extra)
        col_x[c] = col_x[c - 1] + col_widths[c - 1] + gap

    # Compute X coordinate of every node (right-aligned sources, centered middle, left-aligned sinks)
    x_of = {}
    for n in nodes:
        c = col_of[n.key]
        if has_sources and c == 0:
            x_of[n.key] = col_x[c] + (col_widths[c] - n.width)
        elif has_sinks and c == sink_col:
            x_of[n.key] = col_x[c]
        else:
            x_of[n.key] = col_x[c] + (col_widths[c] - n.width) * 0.5

    # Layout each connected component vertically in its own non-overlapping horizontal band
    y_of = {}
    y_cursor = 0.0

    for comp in components:
        comp_edges = [e for e in valid_edges if e.src_key in comp and e.dst_key in comp]
        comp_cols = sorted({col_of[k] for k in comp})
        min_c, max_c = comp_cols[0], comp_cols[-1]

        items_by_key = {}
        orders = {c: [] for c in range(min_c, max_c + 1)}

        for k in sorted(comp, key=lambda key: (nodes_by_key[key].name.lower(), str(key))):
            n = nodes_by_key[k]
            items_by_key[k] = _ColumnItem(key=k, height=n.height, is_dummy=False, sort_name=n.name.lower())
            orders[col_of[k]].append(k)

        # Group forward edges by (src_key, dst_key) to create shared routing corridors for long edges
        forward_bundles = defaultdict(list)
        for e in comp_edges:
            if col_of[e.src_key] < col_of[e.dst_key]:
                forward_bundles[(e.src_key, e.dst_key)].append(e)

        segs_by_gap = defaultdict(list)

        for (src_k, dst_k), bundle_edges in sorted(
            forward_bundles.items(),
            key=lambda kv: (nodes_by_key[kv[0][0]].name.lower(), nodes_by_key[kv[0][1]].name.lower()),
        ):
            c_src = col_of[src_k]
            c_dst = col_of[dst_k]
            sn = nodes_by_key[src_k]
            dn = nodes_by_key[dst_k]

            if c_dst == c_src + 1:
                for e in bundle_edges:
                    s_off = sn.out_port_y.get(e.src_port_id, sn.height * 0.5)
                    d_off = dn.in_port_y.get(e.dst_port_id, dn.height * 0.5)
                    segs_by_gap[c_src].append(_Segment(src_k, dst_k, s_off, d_off, 1))
            else:
                corridor_h = max(24.0, 18.0 + 6.0 * len(bundle_edges))
                dummy_keys = []
                for c in range(c_src + 1, c_dst):
                    dk = ("__dummy__", src_k, dst_k, c)
                    items_by_key[dk] = _ColumnItem(
                        key=dk,
                        height=corridor_h,
                        is_dummy=True,
                        sort_name=f"{sn.name.lower()}->{dn.name.lower()}",
                        bundle=(src_k, dst_k),
                    )
                    orders[c].append(dk)
                    dummy_keys.append(dk)

                # First segment: real src ports -> first dummy corridor
                first_dk = dummy_keys[0]
                for e in bundle_edges:
                    s_off = sn.out_port_y.get(e.src_port_id, sn.height * 0.5)
                    segs_by_gap[c_src].append(_Segment(src_k, first_dk, s_off, corridor_h * 0.5, 1))

                # Intermediate dummy-to-dummy segments
                for idx in range(len(dummy_keys) - 1):
                    c = c_src + 1 + idx
                    segs_by_gap[c].append(
                        _Segment(dummy_keys[idx], dummy_keys[idx + 1], corridor_h * 0.5, corridor_h * 0.5, len(bundle_edges))
                    )

                # Final segment: last dummy corridor -> real dst ports
                last_dk = dummy_keys[-1]
                for e in bundle_edges:
                    d_off = dn.in_port_y.get(e.dst_port_id, dn.height * 0.5)
                    segs_by_gap[c_dst - 1].append(_Segment(last_dk, dst_k, corridor_h * 0.5, d_off, 1))

        # Minimize port-aware crossings across adjacent columns
        orders = _minimize_crossings(orders, segs_by_gap, items_by_key, v_gap)

        # Assign Y coordinates via PAV horizontal alignment
        comp_y = _assign_component_y(orders, segs_by_gap, items_by_key, v_gap)

        # Geometrically clear any intermediate boxes touched by long curves
        comp_y = _clear_edge_box_overlaps(
            comp, comp_edges, col_of, orders, x_of, comp_y, nodes_by_key, v_gap, use_bezier
        )

        # Normalize component top to y_cursor
        min_y = min(comp_y[k] for k in comp)
        max_bottom = 0.0
        for k in comp:
            ny = y_cursor + (comp_y[k] - min_y)
            y_of[k] = ny
            bottom = ny + nodes_by_key[k].height
            if bottom > max_bottom:
                max_bottom = bottom

        y_cursor = max_bottom + comp_gap

    # Place isolated (unconnected) nodes neatly below connected components in their role columns
    if isolated:
        iso_by_col = defaultdict(list)
        for k in isolated:
            iso_by_col[col_of[k]].append(k)

        for c, keys in iso_by_col.items():
            keys.sort(key=lambda key: (nodes_by_key[key].name.lower(), str(key)))
            col_cursor = y_cursor
            for k in keys:
                y_of[k] = col_cursor
                col_cursor += nodes_by_key[k].height + v_gap

    return {n.key: (x_of[n.key], y_of[n.key]) for n in nodes}


def compute_layout(nodes, edges, canvas_rect=None, use_bezier=True):
    """Compute clean left-to-right grid positions for PatchCanvas boxes.

    Parameters
    ----------
    nodes : list[LayoutNode]
        Visible canvas boxes with dimensions and port Y offsets.
    edges : list[LayoutEdge]
        Port-to-port connections between boxes.
    canvas_rect : tuple[float, float, float, float] | None
        Optional (x, y, width, height) of the canvas scene rect for centering and fit.
    use_bezier : bool
        Whether PatchCanvas renders cubic Bezier lines (True) or straight lines (False).

    Returns
    -------
    dict[object, tuple[int, int]]
        Mapping from LayoutNode.key to integer (x, y) scene coordinates.
    """
    if not nodes:
        return {}

    nodes_by_key = {n.key: n for n in nodes}
    h_gap = BASE_H_GAP
    v_gap = BASE_V_GAP
    comp_gap = BASE_COMP_GAP

    positions = _layout_with_spacing(nodes, edges, h_gap, v_gap, comp_gap, use_bezier)

    # Tighten spacing gently if the layout exceeds the canvas bounds
    if canvas_rect is not None:
        rx, ry, rw, rh = canvas_rect
        avail_w = max(1.0, rw - 2.0 * CANVAS_MARGIN)
        avail_h = max(1.0, rh - 2.0 * CANVAS_MARGIN)

        total_w = max(positions[k][0] + nodes_by_key[k].width for k in positions)
        total_h = max(positions[k][1] + nodes_by_key[k].height for k in positions)

        if total_w > avail_w or total_h > avail_h:
            scale_x = avail_w / total_w if total_w > avail_w else 1.0
            scale_y = avail_h / total_h if total_h > avail_h else 1.0
            h_gap = max(MIN_H_GAP, BASE_H_GAP * scale_x)
            v_gap = max(MIN_V_GAP, BASE_V_GAP * scale_y)
            comp_gap = max(MIN_COMP_GAP, BASE_COMP_GAP * scale_y)
            positions = _layout_with_spacing(nodes, edges, h_gap, v_gap, comp_gap, use_bezier)

        min_x = min(positions[k][0] for k in positions)
        min_y = min(positions[k][1] for k in positions)
        max_x = max(positions[k][0] + nodes_by_key[k].width for k in positions)
        max_y = max(positions[k][1] + nodes_by_key[k].height for k in positions)
        total_w = max_x - min_x
        total_h = max_y - min_y

        offset_x = rx + max(CANVAS_MARGIN, (rw - total_w) * 0.5) - min_x
        offset_y = ry + max(CANVAS_MARGIN, (rh - total_h) * 0.5) - min_y

        # Clamp within canvas bounds while keeping coordinates >= rx + 1, ry + 1
        max_off_x = rx + rw - CANVAS_MARGIN - max_x
        max_off_y = ry + rh - CANVAS_MARGIN - max_y
        if offset_x > max_off_x:
            offset_x = max(rx + 1.0 - min_x, max_off_x)
        if offset_y > max_off_y:
            offset_y = max(ry + 1.0 - min_y, max_off_y)
    else:
        offset_x = CANVAS_MARGIN
        offset_y = CANVAS_MARGIN

    result = {}
    for k, (x, y) in positions.items():
        ix = int(round(x + offset_x))
        iy = int(round(y + offset_y))
        if canvas_rect is not None:
            rx, ry, rw, rh = canvas_rect
            n = nodes_by_key[k]
            ix = max(int(rx) + 1, min(ix, int(rx + rw - n.width)))
            iy = max(int(ry) + 1, min(iy, int(ry + rh - n.height)))
        result[k] = (ix, iy)

    return result
