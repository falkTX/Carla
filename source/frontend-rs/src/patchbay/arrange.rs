// SPDX-License-Identifier: GPL-2.0-or-later

//! Left-to-right grid auto-arrangement for PatchCanvas nodes.
//! Direct port of `source/frontend/patchcanvas/arrange.py`.

#![allow(dead_code)]

use std::collections::{BTreeMap, HashMap, HashSet, VecDeque};

pub const ROLE_SOURCE: u8 = 0; // Output-only nodes (far left)
pub const ROLE_MIDDLE: u8 = 1; // Nodes with both inputs and outputs, or no ports
pub const ROLE_SINK: u8 = 2; // Input-only nodes (far right)

pub const BASE_H_GAP: f64 = 120.0;
pub const MIN_H_GAP: f64 = 64.0;
pub const MAX_H_GAP: f64 = 240.0;
pub const BASE_V_GAP: f64 = 28.0;
pub const MIN_V_GAP: f64 = 14.0;
pub const BASE_COMP_GAP: f64 = 48.0;
pub const MIN_COMP_GAP: f64 = 24.0;
pub const CANVAS_MARGIN: f64 = 40.0;
pub const CLEARANCE_PAD_X: f64 = 6.0;
pub const CLEARANCE_PAD_Y: f64 = 12.0;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct NodeKey {
    pub group_id: u32,
    pub split_part: u8, // 0 = joined/output, 1 = split input
}

#[derive(Clone, Debug)]
pub struct LayoutNode {
    pub key: NodeKey,
    pub name: String,
    pub width: f64,
    pub height: f64,
    pub has_inputs: bool,
    pub has_outputs: bool,
    pub in_port_y: HashMap<u32, f64>,
    pub out_port_y: HashMap<u32, f64>,
}

impl LayoutNode {
    pub fn role(&self) -> u8 {
        if self.has_outputs && !self.has_inputs {
            ROLE_SOURCE
        } else if self.has_inputs && !self.has_outputs {
            ROLE_SINK
        } else {
            ROLE_MIDDLE
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct LayoutEdge {
    pub src_key: NodeKey,
    pub src_port_id: u32,
    pub dst_key: NodeKey,
    pub dst_port_id: u32,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, PartialOrd, Ord)]
enum ItemKey {
    Real(NodeKey),
    Dummy(NodeKey, NodeKey, usize),
}

#[derive(Clone, Debug)]
struct ColumnItem {
    key: ItemKey,
    height: f64,
    sort_name: String,
}

#[derive(Clone, Copy, Debug)]
struct Segment {
    src_item: ItemKey,
    dst_item: ItemKey,
    src_offset: f64,
    dst_offset: f64,
    weight: usize,
}

fn break_cycles_greedy(
    node_keys: &HashSet<NodeKey>,
    weighted_edges: &HashMap<(NodeKey, NodeKey), usize>,
    nodes_by_key: &HashMap<NodeKey, &LayoutNode>,
    left_bias: &HashMap<NodeKey, usize>,
    right_bias: &HashMap<NodeKey, usize>,
) -> HashSet<(NodeKey, NodeKey)> {
    let mut remaining: HashSet<NodeKey> = node_keys.clone();
    let mut adj_out: HashMap<NodeKey, HashMap<NodeKey, usize>> =
        remaining.iter().map(|&k| (k, HashMap::new())).collect();
    let mut adj_in: HashMap<NodeKey, HashMap<NodeKey, usize>> =
        remaining.iter().map(|&k| (k, HashMap::new())).collect();

    for (&(u, v), &w) in weighted_edges {
        if remaining.contains(&u) && remaining.contains(&v) && u != v {
            adj_out.get_mut(&u).unwrap().insert(v, w);
            adj_in.get_mut(&v).unwrap().insert(u, w);
        }
    }

    let sort_key = |k: NodeKey| (nodes_by_key[&k].name.to_lowercase(), k);

    let mut left = Vec::new();
    let mut right = Vec::new();

    while !remaining.is_empty() {
        let mut progressed = true;
        while progressed {
            progressed = false;
            let mut sources: Vec<NodeKey> = remaining
                .iter()
                .copied()
                .filter(|k| adj_in[k].is_empty())
                .collect();
            sources.sort_by_key(|&k| sort_key(k));
            for u in sources {
                left.push(u);
                remaining.remove(&u);
                let outs: Vec<NodeKey> = adj_out[&u].keys().copied().collect();
                for v in outs {
                    if let Some(m) = adj_in.get_mut(&v) {
                        m.remove(&u);
                    }
                }
                adj_out.get_mut(&u).unwrap().clear();
                progressed = true;
            }

            let mut sinks: Vec<NodeKey> = remaining
                .iter()
                .copied()
                .filter(|k| adj_out[k].is_empty())
                .collect();
            sinks.sort_by_key(|&k| sort_key(k));
            for v in sinks {
                right.push(v);
                remaining.remove(&v);
                let ins: Vec<NodeKey> = adj_in[&v].keys().copied().collect();
                for u in ins {
                    if let Some(m) = adj_out.get_mut(&u) {
                        m.remove(&v);
                    }
                }
                adj_in.get_mut(&v).unwrap().clear();
                progressed = true;
            }
        }

        if remaining.is_empty() {
            break;
        }

        let best = *remaining
            .iter()
            .min_by_key(|&&k| {
                let out_w: isize = adj_out[&k].values().copied().sum::<usize>() as isize;
                let in_w: isize = adj_in[&k].values().copied().sum::<usize>() as isize;
                let lb = *left_bias.get(&k).unwrap_or(&0) as isize;
                let rb = *right_bias.get(&k).unwrap_or(&0) as isize;
                (-(out_w - in_w + 2 * lb - 2 * rb), adj_in[&k].len(), sort_key(k))
            })
            .unwrap();

        left.push(best);
        remaining.remove(&best);
        let outs: Vec<NodeKey> = adj_out[&best].keys().copied().collect();
        for v in outs {
            if let Some(m) = adj_in.get_mut(&v) {
                m.remove(&best);
            }
        }
        let ins: Vec<NodeKey> = adj_in[&best].keys().copied().collect();
        for u in ins {
            if let Some(m) = adj_out.get_mut(&u) {
                m.remove(&best);
            }
        }
        adj_out.get_mut(&best).unwrap().clear();
        adj_in.get_mut(&best).unwrap().clear();
    }

    right.reverse();
    left.extend(right);
    let rank: HashMap<NodeKey, usize> = left.into_iter().enumerate().map(|(i, k)| (k, i)).collect();

    weighted_edges
        .keys()
        .copied()
        .filter(|&(u, v)| {
            rank.get(&u)
                .zip(rank.get(&v))
                .is_some_and(|(ru, rv)| ru > rv)
        })
        .collect()
}

fn assign_middle_layers(
    middle_keys: &[NodeKey],
    edges_all: &[LayoutEdge],
    nodes_by_key: &HashMap<NodeKey, &LayoutNode>,
) -> HashMap<NodeKey, usize> {
    let middle_set: HashSet<NodeKey> = middle_keys.iter().copied().collect();
    if middle_set.is_empty() {
        return HashMap::new();
    }

    let mut weighted_mid: HashMap<(NodeKey, NodeKey), usize> = HashMap::new();
    let mut source_in_w: HashMap<NodeKey, usize> = HashMap::new();
    let mut sink_out_w: HashMap<NodeKey, usize> = HashMap::new();

    for e in edges_all {
        if e.src_key == e.dst_key {
            continue;
        }
        if middle_set.contains(&e.src_key) && middle_set.contains(&e.dst_key) {
            *weighted_mid.entry((e.src_key, e.dst_key)).or_insert(0) += 1;
        } else if middle_set.contains(&e.dst_key) && nodes_by_key[&e.src_key].role() == ROLE_SOURCE
        {
            *source_in_w.entry(e.dst_key).or_insert(0) += 1;
        } else if middle_set.contains(&e.src_key) && nodes_by_key[&e.dst_key].role() == ROLE_SINK {
            *sink_out_w.entry(e.src_key).or_insert(0) += 1;
        }
    }

    let reversed_edges = break_cycles_greedy(
        &middle_set,
        &weighted_mid,
        nodes_by_key,
        &source_in_w,
        &sink_out_w,
    );

    let mut dag_out: HashMap<NodeKey, Vec<NodeKey>> =
        middle_set.iter().map(|&k| (k, Vec::new())).collect();
    let mut dag_in: HashMap<NodeKey, Vec<NodeKey>> =
        middle_set.iter().map(|&k| (k, Vec::new())).collect();
    let mut indeg: HashMap<NodeKey, usize> = middle_set.iter().map(|&k| (k, 0)).collect();

    for &(mut u, mut v) in weighted_mid.keys() {
        if reversed_edges.contains(&(u, v)) {
            std::mem::swap(&mut u, &mut v);
        }
        if !dag_out[&u].contains(&v) {
            dag_out.get_mut(&u).unwrap().push(v);
            dag_in.get_mut(&v).unwrap().push(u);
            *indeg.get_mut(&v).unwrap() += 1;
        }
    }

    let mut zero_in: Vec<NodeKey> = indeg
        .iter()
        .filter(|&(_, &d)| d == 0)
        .map(|(&k, _)| k)
        .collect();
    zero_in.sort_by_key(|&k| (nodes_by_key[&k].name.to_lowercase(), k));

    let mut queue: VecDeque<NodeKey> = zero_in.into();
    let mut topo = Vec::new();
    while let Some(u) = queue.pop_front() {
        topo.push(u);
        for &v in &dag_out[&u] {
            let d = indeg.get_mut(&v).unwrap();
            *d -= 1;
            if *d == 0 {
                queue.push_back(v);
            }
        }
    }

    let mut layers: HashMap<NodeKey, usize> = middle_set.iter().map(|&k| (k, 0)).collect();
    for &u in &topo {
        let lu = layers[&u];
        for &v in &dag_out[&u] {
            let lv = layers.get_mut(&v).unwrap();
            if *lv < lu + 1 {
                *lv = lu + 1;
            }
        }
    }

    // Tighten middle nodes without incoming edges closer to downstream targets
    for &u in topo.iter().rev() {
        if dag_in[&u].is_empty()
            && *source_in_w.get(&u).unwrap_or(&0) == 0
            && !dag_out[&u].is_empty()
        {
            if let Some(min_next) = dag_out[&u].iter().map(|v| layers[v]).min() {
                if min_next > 0 {
                    layers.insert(u, min_next - 1);
                }
            }
        }
    }

    layers
}

fn pack_column_pav(desired: &[f64], heights: &[f64], v_gap: f64) -> Vec<f64> {
    let n = desired.len();
    if n == 0 {
        return Vec::new();
    }
    let mut offsets = vec![0.0; n];
    for i in 1..n {
        offsets[i] = offsets[i - 1] + heights[i - 1] + v_gap;
    }

    // (start, end, sum_val, count)
    let mut blocks: Vec<(usize, usize, f64, usize)> = Vec::with_capacity(n);
    for i in 0..n {
        let val = desired[i] - offsets[i];
        blocks.push((i, i, val, 1));
        while blocks.len() >= 2 {
            let len = blocks.len();
            let prev = blocks[len - 2];
            let curr = blocks[len - 1];
            if prev.2 / (prev.3 as f64) > curr.2 / (curr.3 as f64) {
                blocks[len - 2] = (prev.0, curr.1, prev.2 + curr.2, prev.3 + curr.3);
                blocks.pop();
            } else {
                break;
            }
        }
    }

    let mut y = vec![0.0; n];
    for (start, end, total, count) in blocks {
        let mean_z = total / (count as f64);
        for i in start..=end {
            y[i] = mean_z + offsets[i];
        }
    }
    y
}

fn count_pair_crossings(
    left_order: &[ItemKey],
    right_order: &[ItemKey],
    segments: &[Segment],
    items_by_key: &HashMap<ItemKey, ColumnItem>,
    v_gap: f64,
) -> (usize, f64) {
    if segments.is_empty() {
        return (0, 0.0);
    }

    let mut left_y = HashMap::with_capacity(left_order.len());
    let mut cursor = 0.0;
    for &k in left_order {
        left_y.insert(k, cursor);
        cursor += items_by_key[&k].height + v_gap;
    }

    let mut right_y = HashMap::with_capacity(right_order.len());
    cursor = 0.0;
    for &k in right_order {
        right_y.insert(k, cursor);
        cursor += items_by_key[&k].height + v_gap;
    }

    let mut coords = Vec::with_capacity(segments.len());
    let mut total_span = 0.0;
    for seg in segments {
        let yl = left_y[&seg.src_item] + seg.src_offset;
        let yr = right_y[&seg.dst_item] + seg.dst_offset;
        coords.push((yl, yr, seg.weight));
        total_span += (yl - yr).abs() * seg.weight as f64;
    }

    let mut crossings = 0usize;
    let m = coords.len();
    for i in 0..m {
        let (yl1, yr1, w1) = coords[i];
        for j in (i + 1)..m {
            let (yl2, yr2, w2) = coords[j];
            if (yl1 - yl2) * (yr1 - yr2) < -1e-6 {
                crossings += w1 * w2;
            }
        }
    }
    (crossings, total_span)
}

fn total_crossings(
    orders: &BTreeMap<usize, Vec<ItemKey>>,
    segs_by_gap: &HashMap<usize, Vec<Segment>>,
    items_by_key: &HashMap<ItemKey, ColumnItem>,
    v_gap: f64,
) -> (usize, f64) {
    let mut cross = 0usize;
    let mut span = 0.0;
    for (&c, segs) in segs_by_gap {
        if !segs.is_empty() {
            if let (Some(left), Some(right)) = (orders.get(&c), orders.get(&(c + 1))) {
                let (cr, sp) = count_pair_crossings(left, right, segs, items_by_key, v_gap);
                cross += cr;
                span += sp;
            }
        }
    }
    (cross, span)
}

fn better_cost(a: (usize, f64), b: (usize, f64)) -> bool {
    a.0 < b.0 || (a.0 == b.0 && a.1 + 1e-6 < b.1)
}

fn for_each_permutation<T: Clone>(items: &mut [T], start: usize, f: &mut impl FnMut(&[T])) {
    if start == items.len() {
        f(items);
        return;
    }
    for i in start..items.len() {
        items.swap(start, i);
        for_each_permutation(items, start + 1, f);
        items.swap(start, i);
    }
}

fn minimize_crossings(
    mut orders: BTreeMap<usize, Vec<ItemKey>>,
    segs_by_gap: &HashMap<usize, Vec<Segment>>,
    items_by_key: &HashMap<ItemKey, ColumnItem>,
    v_gap: f64,
) -> BTreeMap<usize, Vec<ItemKey>> {
    let cols: Vec<usize> = orders.keys().copied().collect();
    if cols.len() <= 1 {
        return orders;
    }

    let mut in_segs: HashMap<ItemKey, Vec<Segment>> = HashMap::new();
    let mut out_segs: HashMap<ItemKey, Vec<Segment>> = HashMap::new();
    for segs in segs_by_gap.values() {
        for &seg in segs {
            out_segs.entry(seg.src_item).or_default().push(seg);
            in_segs.entry(seg.dst_item).or_default().push(seg);
        }
    }

    let col_y_map = |order: &[ItemKey]| -> HashMap<ItemKey, f64> {
        let mut ymap = HashMap::with_capacity(order.len());
        let mut cursor = 0.0;
        for &k in order {
            ymap.insert(k, cursor);
            cursor += items_by_key[&k].height + v_gap;
        }
        ymap
    };

    let column_local_cost = |c: usize,
                             cand_order: &[ItemKey],
                             orders: &BTreeMap<usize, Vec<ItemKey>>|
     -> (usize, f64) {
        let mut cross = 0;
        let mut span = 0.0;
        if c > 0 {
            if let Some(segs) = segs_by_gap.get(&(c - 1)) {
                if !segs.is_empty() {
                    let (cr, sp) =
                        count_pair_crossings(&orders[&(c - 1)], cand_order, segs, items_by_key, v_gap);
                    cross += cr;
                    span += sp;
                }
            }
        }
        if let Some(segs) = segs_by_gap.get(&c) {
            if !segs.is_empty() {
                let (cr, sp) =
                    count_pair_crossings(cand_order, &orders[&(c + 1)], segs, items_by_key, v_gap);
                cross += cr;
                span += sp;
            }
        }
        (cross, span)
    };

    let optimize_column = |c: usize, orders: &mut BTreeMap<usize, Vec<ItemKey>>| {
        let order = orders[&c].clone();
        if order.len() <= 1 {
            return;
        }
        if order.len() <= 6 {
            let mut best_order = order.clone();
            let mut best_cost = column_local_cost(c, &order, orders);
            let mut scratch = order;
            for_each_permutation(&mut scratch, 0, &mut |cand| {
                let cost = column_local_cost(c, cand, orders);
                if better_cost(cost, best_cost) {
                    best_cost = cost;
                    best_order = cand.to_vec();
                }
            });
            orders.insert(c, best_order);
            return;
        }

        let mut cur = order;
        let mut improved = true;
        while improved {
            improved = false;
            let mut best_cost = column_local_cost(c, &cur, orders);
            for i in 0..(cur.len() - 1) {
                cur.swap(i, i + 1);
                let cand_cost = column_local_cost(c, &cur, orders);
                if better_cost(cand_cost, best_cost) {
                    best_cost = cand_cost;
                    improved = true;
                } else {
                    cur.swap(i, i + 1);
                }
            }
        }
        orders.insert(c, cur);
    };

    let mut best_orders = orders.clone();
    let mut best_score = total_crossings(&orders, segs_by_gap, items_by_key, v_gap);

    for _ in 0..10 {
        let prev_best = best_score;

        // L -> R barycenter sweep
        for &c in &cols[1..] {
            if orders[&c].len() <= 1 {
                continue;
            }
            let prev_y = col_y_map(&orders[&(c - 1)]);
            let curr_idx: HashMap<ItemKey, usize> = orders[&c]
                .iter()
                .enumerate()
                .map(|(i, &k)| (k, i))
                .collect();
            let col_vec = orders.get_mut(&c).unwrap();
            col_vec.sort_by(|a, b| {
                let key = |item_key: ItemKey| -> (f64, &str) {
                    if let Some(segs) = in_segs.get(&item_key) {
                        let tw: usize = segs.iter().map(|s| s.weight).sum();
                        if tw > 0 {
                            let sum_y: f64 = segs
                                .iter()
                                .map(|s| (prev_y[&s.src_item] + s.src_offset) * s.weight as f64)
                                .sum();
                            return (sum_y / tw as f64, &items_by_key[&item_key].sort_name);
                        }
                    }
                    (
                        curr_idx[&item_key] as f64 * 100.0,
                        &items_by_key[&item_key].sort_name,
                    )
                };
                let ka = key(*a);
                let kb = key(*b);
                ka.0.total_cmp(&kb.0).then_with(|| ka.1.cmp(kb.1))
            });
        }
        let s = total_crossings(&orders, segs_by_gap, items_by_key, v_gap);
        if better_cost(s, best_score) {
            best_score = s;
            best_orders = orders.clone();
        }

        for &c in &cols {
            optimize_column(c, &mut orders);
        }
        let s = total_crossings(&orders, segs_by_gap, items_by_key, v_gap);
        if better_cost(s, best_score) {
            best_score = s;
            best_orders = orders.clone();
        }

        // R -> L barycenter sweep
        for &c in cols[..cols.len() - 1].iter().rev() {
            if orders[&c].len() <= 1 {
                continue;
            }
            let next_y = col_y_map(&orders[&(c + 1)]);
            let curr_idx: HashMap<ItemKey, usize> = orders[&c]
                .iter()
                .enumerate()
                .map(|(i, &k)| (k, i))
                .collect();
            let col_vec = orders.get_mut(&c).unwrap();
            col_vec.sort_by(|a, b| {
                let key = |item_key: ItemKey| -> (f64, &str) {
                    if let Some(segs) = out_segs.get(&item_key) {
                        let tw: usize = segs.iter().map(|s| s.weight).sum();
                        if tw > 0 {
                            let sum_y: f64 = segs
                                .iter()
                                .map(|s| (next_y[&s.dst_item] + s.dst_offset) * s.weight as f64)
                                .sum();
                            return (sum_y / tw as f64, &items_by_key[&item_key].sort_name);
                        }
                    }
                    (
                        curr_idx[&item_key] as f64 * 100.0,
                        &items_by_key[&item_key].sort_name,
                    )
                };
                let ka = key(*a);
                let kb = key(*b);
                ka.0.total_cmp(&kb.0).then_with(|| ka.1.cmp(kb.1))
            });
        }
        let s = total_crossings(&orders, segs_by_gap, items_by_key, v_gap);
        if better_cost(s, best_score) {
            best_score = s;
            best_orders = orders.clone();
        }

        for &c in cols.iter().rev() {
            optimize_column(c, &mut orders);
        }
        let score = total_crossings(&orders, segs_by_gap, items_by_key, v_gap);
        if better_cost(score, best_score) {
            best_score = score;
            best_orders = orders.clone();
        }

        if score.0 == 0 && (best_score.0 == prev_best.0 && (best_score.1 - prev_best.1).abs() < 1e-9)
        {
            break;
        }
    }

    best_orders
}

fn bezier_x(t: f64, x1: f64, x2: f64) -> f64 {
    let mx = 0.5 * (x1 + x2);
    let u = 1.0 - t;
    u * u * u * x1 + 3.0 * u * u * t * mx + 3.0 * u * t * t * mx + t * t * t * x2
}

fn bezier_t_for_x(target_x: f64, x1: f64, x2: f64) -> f64 {
    if target_x <= x1 {
        return 0.0;
    }
    if target_x >= x2 {
        return 1.0;
    }
    let mut lo = 0.0;
    let mut hi = 1.0;
    for _ in 0..16 {
        let mid = 0.5 * (lo + hi);
        if bezier_x(mid, x1, x2) < target_x {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    0.5 * (lo + hi)
}

pub fn edge_y_range_over_x(
    x1: f64,
    y1: f64,
    x2: f64,
    y2: f64,
    bx_left: f64,
    bx_right: f64,
    use_bezier: bool,
) -> Option<(f64, f64)> {
    if x2 <= x1 || bx_right <= x1 || bx_left >= x2 {
        return None;
    }
    let xa = x1.max(bx_left);
    let xb = x2.min(bx_right);
    if xb <= xa {
        return None;
    }
    let (ya, yb) = if use_bezier {
        let ta = bezier_t_for_x(xa, x1, x2);
        let tb = bezier_t_for_x(xb, x1, x2);
        (
            y1 + (y2 - y1) * (3.0 * ta * ta - 2.0 * ta * ta * ta),
            y1 + (y2 - y1) * (3.0 * tb * tb - 2.0 * tb * tb * tb),
        )
    } else {
        let dx = x2 - x1;
        (
            y1 + (y2 - y1) * ((xa - x1) / dx),
            y1 + (y2 - y1) * ((xb - x1) / dx),
        )
    };
    if ya <= yb {
        Some((ya, yb))
    } else {
        Some((yb, ya))
    }
}

fn median_f64(vals: &mut [f64]) -> f64 {
    vals.sort_by(|a, b| a.total_cmp(b));
    let n = vals.len();
    if n % 2 == 1 {
        vals[n / 2]
    } else {
        0.5 * (vals[n / 2 - 1] + vals[n / 2])
    }
}

fn assign_component_y(
    orders: &BTreeMap<usize, Vec<ItemKey>>,
    segs_by_gap: &HashMap<usize, Vec<Segment>>,
    items_by_key: &HashMap<ItemKey, ColumnItem>,
    v_gap: f64,
) -> HashMap<ItemKey, f64> {
    let cols: Vec<usize> = orders.keys().copied().collect();
    let mut in_segs: HashMap<ItemKey, Vec<Segment>> = HashMap::new();
    let mut out_segs: HashMap<ItemKey, Vec<Segment>> = HashMap::new();
    for segs in segs_by_gap.values() {
        for &seg in segs {
            out_segs.entry(seg.src_item).or_default().push(seg);
            in_segs.entry(seg.dst_item).or_default().push(seg);
        }
    }

    let mut y_pos = HashMap::new();
    for &c in &cols {
        let mut cursor = 0.0;
        for &k in &orders[&c] {
            y_pos.insert(k, cursor);
            cursor += items_by_key[&k].height + v_gap;
        }
    }

    let relax_column =
        |c: usize, use_left: bool, use_right: bool, y_pos: &mut HashMap<ItemKey, f64>| {
            let order = &orders[&c];
            if order.is_empty() {
                return;
            }
            let mut desired = Vec::with_capacity(order.len());
            let mut heights = Vec::with_capacity(order.len());
            for &k in order {
                heights.push(items_by_key[&k].height);
                let mut targets = Vec::new();
                if use_left {
                    if let Some(segs) = in_segs.get(&k) {
                        for s in segs {
                            let ty = y_pos[&s.src_item] + s.src_offset - s.dst_offset;
                            for _ in 0..s.weight {
                                targets.push(ty);
                            }
                        }
                    }
                }
                if use_right {
                    if let Some(segs) = out_segs.get(&k) {
                        for s in segs {
                            let ty = y_pos[&s.dst_item] + s.dst_offset - s.src_offset;
                            for _ in 0..s.weight {
                                targets.push(ty);
                            }
                        }
                    }
                }
                if targets.is_empty() {
                    desired.push(y_pos[&k]);
                } else {
                    desired.push(median_f64(&mut targets));
                }
            }
            let packed = pack_column_pav(&desired, &heights, v_gap);
            for (&k, yv) in order.iter().zip(packed) {
                y_pos.insert(k, yv);
            }
        };

    for _ in 0..4 {
        for &c in &cols[1..] {
            relax_column(c, true, false, &mut y_pos);
        }
        for &c in cols[..cols.len().saturating_sub(1)].iter().rev() {
            relax_column(c, false, true, &mut y_pos);
        }
    }

    for _ in 0..4 {
        for &c in &cols {
            relax_column(c, true, true, &mut y_pos);
        }
    }

    y_pos
}

#[allow(clippy::too_many_arguments)]
fn clear_edge_box_overlaps(
    comp_nodes: &HashSet<NodeKey>,
    comp_edges: &[LayoutEdge],
    col_of: &HashMap<NodeKey, usize>,
    orders: &BTreeMap<usize, Vec<ItemKey>>,
    x_of: &HashMap<NodeKey, f64>,
    y_pos: HashMap<ItemKey, f64>,
    nodes_by_key: &HashMap<NodeKey, &LayoutNode>,
    v_gap: f64,
    use_bezier: bool,
) -> HashMap<NodeKey, f64> {
    let mut real_y: HashMap<NodeKey, f64> = comp_nodes
        .iter()
        .map(|&k| (k, y_pos[&ItemKey::Real(k)]))
        .collect();

    let forward_edges: Vec<LayoutEdge> = comp_edges
        .iter()
        .copied()
        .filter(|e| {
            comp_nodes.contains(&e.src_key)
                && comp_nodes.contains(&e.dst_key)
                && col_of[&e.dst_key] > col_of[&e.src_key] + 1
        })
        .collect();

    if forward_edges.is_empty() {
        return real_y;
    }

    let mut rank_in_col: HashMap<ItemKey, usize> = HashMap::new();
    let mut real_order_by_col: HashMap<usize, Vec<NodeKey>> = HashMap::new();
    for (&c, order) in orders {
        let mut reals = Vec::new();
        for (idx, &ik) in order.iter().enumerate() {
            rank_in_col.insert(ik, idx);
            if let ItemKey::Real(nk) = ik {
                if comp_nodes.contains(&nk) {
                    reals.push(nk);
                }
            }
        }
        real_order_by_col.insert(c, reals);
    }

    let count_collisions = |cur_y: &HashMap<NodeKey, f64>| -> Vec<(usize, NodeKey, LayoutEdge, f64, f64)> {
        let mut hits = Vec::new();
        for &e in &forward_edges {
            let sn = nodes_by_key[&e.src_key];
            let dn = nodes_by_key[&e.dst_key];
            let x1 = x_of[&e.src_key] + sn.width;
            let y1 = cur_y[&e.src_key]
                + sn.out_port_y
                    .get(&e.src_port_id)
                    .copied()
                    .unwrap_or(sn.height * 0.5);
            let x2 = x_of[&e.dst_key];
            let y2 = cur_y[&e.dst_key]
                + dn.in_port_y
                    .get(&e.dst_port_id)
                    .copied()
                    .unwrap_or(dn.height * 0.5);

            for c in (col_of[&e.src_key] + 1)..col_of[&e.dst_key] {
                if let Some(reals) = real_order_by_col.get(&c) {
                    for &bk in reals {
                        let bn = nodes_by_key[&bk];
                        let bx_l = x_of[&bk] - CLEARANCE_PAD_X;
                        let bx_r = x_of[&bk] + bn.width + CLEARANCE_PAD_X;
                        if let Some((ey_min, ey_max)) =
                            edge_y_range_over_x(x1, y1, x2, y2, bx_l, bx_r, use_bezier)
                        {
                            let by_top = cur_y[&bk] - CLEARANCE_PAD_Y;
                            let by_bot = cur_y[&bk] + bn.height + CLEARANCE_PAD_Y;
                            if ey_max >= by_top && ey_min <= by_bot {
                                hits.push((c, bk, e, ey_min, ey_max));
                            }
                        }
                    }
                }
            }
        }
        hits
    };

    let mut best_y = real_y.clone();
    let mut best_hits = count_collisions(&best_y).len();
    if best_hits == 0 {
        return best_y;
    }

    for _ in 0..12 {
        let hits = count_collisions(&real_y);
        if hits.is_empty() {
            return real_y;
        }
        let mut col_nudges: HashMap<usize, Vec<(NodeKey, bool, f64)>> = HashMap::new();
        for (c, bk, e, ey_min, ey_max) in hits {
            let bn = nodes_by_key[&bk];
            let dummy_key = ItemKey::Dummy(e.src_key, e.dst_key, c);
            let push_down = match (
                rank_in_col.get(&dummy_key),
                rank_in_col.get(&ItemKey::Real(bk)),
            ) {
                (Some(&dr), Some(&br)) => br > dr,
                _ => {
                    let box_mid = real_y[&bk] + bn.height * 0.5;
                    let edge_mid = 0.5 * (ey_min + ey_max);
                    box_mid >= edge_mid
                }
            };
            let target = if push_down {
                ey_max + CLEARANCE_PAD_Y + 2.0
            } else {
                ey_min - CLEARANCE_PAD_Y - bn.height - 2.0
            };
            col_nudges.entry(c).or_default().push((bk, push_down, target));
        }

        for (c, nudges) in col_nudges {
            let Some(real_col) = real_order_by_col.get(&c) else {
                continue;
            };
            if real_col.is_empty() {
                continue;
            }
            let mut desired: HashMap<NodeKey, f64> =
                real_col.iter().map(|&bk| (bk, real_y[&bk])).collect();
            for (bk, push_down, target) in nudges {
                let d = desired.get_mut(&bk).unwrap();
                if push_down {
                    *d = d.max(target);
                } else {
                    *d = d.min(target);
                }
            }
            let desired_vec: Vec<f64> = real_col.iter().map(|bk| desired[bk]).collect();
            let heights: Vec<f64> = real_col.iter().map(|bk| nodes_by_key[bk].height).collect();
            let packed = pack_column_pav(&desired_vec, &heights, v_gap);
            for (&bk, yv) in real_col.iter().zip(packed) {
                real_y.insert(bk, yv);
            }
        }

        let new_hits = count_collisions(&real_y).len();
        if new_hits < best_hits {
            best_hits = new_hits;
            best_y = real_y.clone();
        }
        if new_hits == 0 {
            return best_y;
        }
    }

    best_y
}

fn layout_with_spacing(
    nodes: &[LayoutNode],
    edges: &[LayoutEdge],
    h_gap_base: f64,
    v_gap: f64,
    comp_gap: f64,
    use_bezier: bool,
) -> HashMap<NodeKey, (f64, f64)> {
    let nodes_by_key: HashMap<NodeKey, &LayoutNode> = nodes.iter().map(|n| (n.key, n)).collect();
    let valid_edges: Vec<LayoutEdge> = edges
        .iter()
        .copied()
        .filter(|e| {
            nodes_by_key.contains_key(&e.src_key)
                && nodes_by_key.contains_key(&e.dst_key)
                && e.src_key != e.dst_key
        })
        .collect();

    let mut undir: HashMap<NodeKey, HashSet<NodeKey>> =
        nodes.iter().map(|n| (n.key, HashSet::new())).collect();
    for e in &valid_edges {
        undir.get_mut(&e.src_key).unwrap().insert(e.dst_key);
        undir.get_mut(&e.dst_key).unwrap().insert(e.src_key);
    }

    let mut sorted_nodes: Vec<&LayoutNode> = nodes.iter().collect();
    sorted_nodes.sort_by_key(|n| (n.role(), n.name.to_lowercase(), n.key));

    let mut visited = HashSet::new();
    let mut components: Vec<HashSet<NodeKey>> = Vec::new();
    let mut isolated: Vec<NodeKey> = Vec::new();

    for n in sorted_nodes {
        if visited.contains(&n.key) {
            continue;
        }
        if undir[&n.key].is_empty() {
            visited.insert(n.key);
            isolated.push(n.key);
            continue;
        }
        let mut comp = HashSet::new();
        let mut q = VecDeque::new();
        q.push_back(n.key);
        visited.insert(n.key);
        while let Some(u) = q.pop_front() {
            comp.insert(u);
            for &v in &undir[&u] {
                if visited.insert(v) {
                    q.push_back(v);
                }
            }
        }
        components.push(comp);
    }

    components.sort_by(|a, b| {
        b.len().cmp(&a.len()).then_with(|| {
            let ma = a
                .iter()
                .map(|k| nodes_by_key[k].name.to_lowercase())
                .min()
                .unwrap_or_default();
            let mb = b
                .iter()
                .map(|k| nodes_by_key[k].name.to_lowercase())
                .min()
                .unwrap_or_default();
            ma.cmp(&mb)
        })
    });

    let has_sources = nodes.iter().any(|n| n.role() == ROLE_SOURCE);
    let has_middle = nodes.iter().any(|n| n.role() == ROLE_MIDDLE);
    let has_sinks = nodes.iter().any(|n| n.role() == ROLE_SINK);
    let middle_offset = usize::from(has_sources);

    let mut mid_layers: HashMap<NodeKey, usize> = HashMap::new();
    for comp in &components {
        let comp_mid: Vec<NodeKey> = comp
            .iter()
            .copied()
            .filter(|k| nodes_by_key[k].role() == ROLE_MIDDLE)
            .collect();
        let comp_edges: Vec<LayoutEdge> = valid_edges
            .iter()
            .copied()
            .filter(|e| comp.contains(&e.src_key) && comp.contains(&e.dst_key))
            .collect();
        mid_layers.extend(assign_middle_layers(&comp_mid, &comp_edges, &nodes_by_key));
    }
    for &k in &isolated {
        if nodes_by_key[&k].role() == ROLE_MIDDLE {
            mid_layers.insert(k, 0);
        }
    }

    let max_mid_layer = mid_layers.values().copied().max().unwrap_or(0);
    let num_mid_cols = if has_middle { max_mid_layer + 1 } else { 0 };
    let sink_col = middle_offset + num_mid_cols;
    let num_cols = sink_col + usize::from(has_sinks);

    let mut col_of: HashMap<NodeKey, usize> = HashMap::new();
    for n in nodes {
        let c = match n.role() {
            ROLE_SOURCE => 0,
            ROLE_SINK => sink_col,
            _ => middle_offset + *mid_layers.get(&n.key).unwrap_or(&0),
        };
        col_of.insert(n.key, c);
    }

    let mut col_widths = vec![0.0_f64; num_cols];
    for n in nodes {
        let c = col_of[&n.key];
        col_widths[c] = col_widths[c].max(n.width);
    }

    let mut gap_traffic = vec![0usize; num_cols.saturating_sub(1)];
    for e in &valid_edges {
        let c1 = col_of[&e.src_key];
        let c2 = col_of[&e.dst_key];
        let (lo, hi) = if c1 < c2 { (c1, c2) } else { (c2, c1) };
        for c in lo..hi {
            gap_traffic[c] += 1;
        }
    }

    let mut col_x = vec![0.0_f64; num_cols];
    for c in 1..num_cols {
        let extra = gap_traffic[c - 1].saturating_sub(4) as f64 * 6.0;
        let gap = MAX_H_GAP.min(h_gap_base + extra);
        col_x[c] = col_x[c - 1] + col_widths[c - 1] + gap;
    }

    let mut x_of: HashMap<NodeKey, f64> = HashMap::new();
    for n in nodes {
        let c = col_of[&n.key];
        let x = if has_sources && c == 0 {
            col_x[c] + (col_widths[c] - n.width)
        } else if has_sinks && c == sink_col {
            col_x[c]
        } else {
            col_x[c] + (col_widths[c] - n.width) * 0.5
        };
        x_of.insert(n.key, x);
    }

    let mut y_of: HashMap<NodeKey, f64> = HashMap::new();
    let mut y_cursor = 0.0_f64;

    for comp in &components {
        let comp_edges: Vec<LayoutEdge> = valid_edges
            .iter()
            .copied()
            .filter(|e| comp.contains(&e.src_key) && comp.contains(&e.dst_key))
            .collect();
        let min_c = comp.iter().map(|k| col_of[k]).min().unwrap();
        let max_c = comp.iter().map(|k| col_of[k]).max().unwrap();

        let mut items_by_key: HashMap<ItemKey, ColumnItem> = HashMap::new();
        let mut orders: BTreeMap<usize, Vec<ItemKey>> =
            (min_c..=max_c).map(|c| (c, Vec::new())).collect();

        let mut comp_sorted: Vec<NodeKey> = comp.iter().copied().collect();
        comp_sorted.sort_by_key(|&k| (nodes_by_key[&k].name.to_lowercase(), k));

        for k in comp_sorted {
            let n = nodes_by_key[&k];
            let ik = ItemKey::Real(k);
            items_by_key.insert(
                ik,
                ColumnItem {
                    key: ik,
                    height: n.height,
                    sort_name: n.name.to_lowercase(),
                },
            );
            orders.get_mut(&col_of[&k]).unwrap().push(ik);
        }

        let mut forward_bundles: BTreeMap<(NodeKey, NodeKey), Vec<LayoutEdge>> = BTreeMap::new();
        for &e in &comp_edges {
            if col_of[&e.src_key] < col_of[&e.dst_key] {
                forward_bundles
                    .entry((e.src_key, e.dst_key))
                    .or_default()
                    .push(e);
            }
        }

        let mut segs_by_gap: HashMap<usize, Vec<Segment>> = HashMap::new();

        let mut bundle_keys: Vec<(NodeKey, NodeKey)> = forward_bundles.keys().copied().collect();
        bundle_keys.sort_by_key(|&(s, d)| {
            (
                nodes_by_key[&s].name.to_lowercase(),
                nodes_by_key[&d].name.to_lowercase(),
                s,
                d,
            )
        });

        for (src_k, dst_k) in bundle_keys {
            let bundle_edges = &forward_bundles[&(src_k, dst_k)];
            let c_src = col_of[&src_k];
            let c_dst = col_of[&dst_k];
            let sn = nodes_by_key[&src_k];
            let dn = nodes_by_key[&dst_k];

            if c_dst == c_src + 1 {
                for e in bundle_edges {
                    let s_off = sn
                        .out_port_y
                        .get(&e.src_port_id)
                        .copied()
                        .unwrap_or(sn.height * 0.5);
                    let d_off = dn
                        .in_port_y
                        .get(&e.dst_port_id)
                        .copied()
                        .unwrap_or(dn.height * 0.5);
                    segs_by_gap.entry(c_src).or_default().push(Segment {
                        src_item: ItemKey::Real(src_k),
                        dst_item: ItemKey::Real(dst_k),
                        src_offset: s_off,
                        dst_offset: d_off,
                        weight: 1,
                    });
                }
            } else {
                let corridor_h = (18.0 + 6.0 * bundle_edges.len() as f64).max(24.0);
                let mut dummy_keys = Vec::new();
                for c in (c_src + 1)..c_dst {
                    let dk = ItemKey::Dummy(src_k, dst_k, c);
                    items_by_key.insert(
                        dk,
                        ColumnItem {
                            key: dk,
                            height: corridor_h,
                            sort_name: format!(
                                "{}->{}",
                                sn.name.to_lowercase(),
                                dn.name.to_lowercase()
                            ),
                        },
                    );
                    orders.get_mut(&c).unwrap().push(dk);
                    dummy_keys.push(dk);
                }

                let first_dk = dummy_keys[0];
                for e in bundle_edges {
                    let s_off = sn
                        .out_port_y
                        .get(&e.src_port_id)
                        .copied()
                        .unwrap_or(sn.height * 0.5);
                    segs_by_gap.entry(c_src).or_default().push(Segment {
                        src_item: ItemKey::Real(src_k),
                        dst_item: first_dk,
                        src_offset: s_off,
                        dst_offset: corridor_h * 0.5,
                        weight: 1,
                    });
                }

                for idx in 0..(dummy_keys.len().saturating_sub(1)) {
                    let c = c_src + 1 + idx;
                    segs_by_gap.entry(c).or_default().push(Segment {
                        src_item: dummy_keys[idx],
                        dst_item: dummy_keys[idx + 1],
                        src_offset: corridor_h * 0.5,
                        dst_offset: corridor_h * 0.5,
                        weight: bundle_edges.len(),
                    });
                }

                let last_dk = *dummy_keys.last().unwrap();
                for e in bundle_edges {
                    let d_off = dn
                        .in_port_y
                        .get(&e.dst_port_id)
                        .copied()
                        .unwrap_or(dn.height * 0.5);
                    segs_by_gap.entry(c_dst - 1).or_default().push(Segment {
                        src_item: last_dk,
                        dst_item: ItemKey::Real(dst_k),
                        src_offset: corridor_h * 0.5,
                        dst_offset: d_off,
                        weight: 1,
                    });
                }
            }
        }

        orders = minimize_crossings(orders, &segs_by_gap, &items_by_key, v_gap);
        let comp_y_items = assign_component_y(&orders, &segs_by_gap, &items_by_key, v_gap);
        let comp_y = clear_edge_box_overlaps(
            comp,
            &comp_edges,
            &col_of,
            &orders,
            &x_of,
            comp_y_items,
            &nodes_by_key,
            v_gap,
            use_bezier,
        );

        let min_y = comp
            .iter()
            .map(|k| comp_y[k])
            .fold(f64::INFINITY, f64::min);
        let mut max_bottom = 0.0_f64;
        for &k in comp {
            let ny = y_cursor + (comp_y[&k] - min_y);
            y_of.insert(k, ny);
            max_bottom = max_bottom.max(ny + nodes_by_key[&k].height);
        }
        y_cursor = max_bottom + comp_gap;
    }

    if !isolated.is_empty() {
        let mut iso_by_col: BTreeMap<usize, Vec<NodeKey>> = BTreeMap::new();
        for &k in &isolated {
            iso_by_col.entry(col_of[&k]).or_default().push(k);
        }
        for (_, mut keys) in iso_by_col {
            keys.sort_by_key(|&k| (nodes_by_key[&k].name.to_lowercase(), k));
            let mut col_cursor = y_cursor;
            for k in keys {
                y_of.insert(k, col_cursor);
                col_cursor += nodes_by_key[&k].height + v_gap;
            }
        }
    }

    nodes
        .iter()
        .map(|n| (n.key, (x_of[&n.key], y_of[&n.key])))
        .collect()
}

pub fn compute_layout(
    nodes: &[LayoutNode],
    edges: &[LayoutEdge],
    canvas_rect: Option<(f64, f64, f64, f64)>,
    use_bezier: bool,
) -> HashMap<NodeKey, (i32, i32)> {
    if nodes.is_empty() {
        return HashMap::new();
    }
    let nodes_by_key: HashMap<NodeKey, &LayoutNode> = nodes.iter().map(|n| (n.key, n)).collect();
    let mut positions = layout_with_spacing(
        nodes,
        edges,
        BASE_H_GAP,
        BASE_V_GAP,
        BASE_COMP_GAP,
        use_bezier,
    );

    let (offset_x, offset_y) = if let Some((rx, ry, rw, rh)) = canvas_rect {
        let avail_w = (rw - 2.0 * CANVAS_MARGIN).max(1.0);
        let avail_h = (rh - 2.0 * CANVAS_MARGIN).max(1.0);

        let total_w = positions
            .iter()
            .map(|(k, &(x, _))| x + nodes_by_key[k].width)
            .fold(0.0_f64, f64::max);
        let total_h = positions
            .iter()
            .map(|(k, &(_, y))| y + nodes_by_key[k].height)
            .fold(0.0_f64, f64::max);

        if total_w > avail_w || total_h > avail_h {
            let scale_x = if total_w > avail_w {
                avail_w / total_w
            } else {
                1.0
            };
            let scale_y = if total_h > avail_h {
                avail_h / total_h
            } else {
                1.0
            };
            let h_gap = MIN_H_GAP.max(BASE_H_GAP * scale_x);
            let v_gap = MIN_V_GAP.max(BASE_V_GAP * scale_y);
            let comp_gap = MIN_COMP_GAP.max(BASE_COMP_GAP * scale_y);
            positions = layout_with_spacing(nodes, edges, h_gap, v_gap, comp_gap, use_bezier);
        }

        let min_x = positions.values().map(|&(x, _)| x).fold(f64::INFINITY, f64::min);
        let min_y = positions.values().map(|&(_, y)| y).fold(f64::INFINITY, f64::min);
        let max_x = positions
            .iter()
            .map(|(k, &(x, _))| x + nodes_by_key[k].width)
            .fold(f64::NEG_INFINITY, f64::max);
        let max_y = positions
            .iter()
            .map(|(k, &(_, y))| y + nodes_by_key[k].height)
            .fold(f64::NEG_INFINITY, f64::max);
        let tw = max_x - min_x;
        let th = max_y - min_y;

        let mut ox = rx + CANVAS_MARGIN.max((rw - tw) * 0.5) - min_x;
        let mut oy = ry + CANVAS_MARGIN.max((rh - th) * 0.5) - min_y;

        let max_ox = rx + rw - CANVAS_MARGIN - max_x;
        let max_oy = ry + rh - CANVAS_MARGIN - max_y;
        if ox > max_ox {
            ox = (rx + 1.0 - min_x).max(max_ox);
        }
        if oy > max_oy {
            oy = (ry + 1.0 - min_y).max(max_oy);
        }
        (ox, oy)
    } else {
        (CANVAS_MARGIN, CANVAS_MARGIN)
    };

    let mut result = HashMap::with_capacity(positions.len());
    for (k, (x, y)) in positions {
        let mut ix = (x + offset_x).round() as i32;
        let mut iy = (y + offset_y).round() as i32;
        if let Some((rx, ry, rw, rh)) = canvas_rect {
            let n = nodes_by_key[&k];
            ix = ix.clamp(rx as i32 + 1, (rx + rw - n.width) as i32);
            iy = iy.clamp(ry as i32 + 1, (ry + rh - n.height) as i32);
        }
        result.insert(k, (ix, iy));
    }
    result
}

#[cfg(test)]
mod tests {
    use super::*;

    fn mk_node(id: u32, name: &str, ins: u32, outs: u32) -> LayoutNode {
        let mut in_y = HashMap::new();
        let mut out_y = HashMap::new();
        for i in 0..ins {
            in_y.insert(i + 1, 28.0 + i as f64 * 18.0);
        }
        for i in 0..outs {
            out_y.insert(100 + i + 1, 28.0 + i as f64 * 18.0);
        }
        let max_p = ins.max(outs).max(1);
        LayoutNode {
            key: NodeKey {
                group_id: id,
                split_part: 0,
            },
            name: name.to_string(),
            width: 160.0,
            height: 30.0 + max_p as f64 * 18.0,
            has_inputs: ins > 0,
            has_outputs: outs > 0,
            in_port_y: in_y,
            out_port_y: out_y,
        }
    }

    #[test]
    fn test_roles_left_middle_right() {
        let nodes = vec![
            mk_node(1, "Mic", 0, 2),
            mk_node(2, "EQ", 2, 2),
            mk_node(3, "Comp", 2, 2),
            mk_node(4, "Speakers", 2, 0),
        ];
        let k = |id| NodeKey { group_id: id, split_part: 0 };
        let edges = vec![
            LayoutEdge { src_key: k(1), src_port_id: 101, dst_key: k(2), dst_port_id: 1 },
            LayoutEdge { src_key: k(2), src_port_id: 101, dst_key: k(3), dst_port_id: 1 },
            LayoutEdge { src_key: k(3), src_port_id: 101, dst_key: k(4), dst_port_id: 1 },
        ];
        let pos = compute_layout(&nodes, &edges, Some((0.0, 0.0, 3100.0, 2400.0)), true);
        assert!(pos[&k(1)].0 < pos[&k(2)].0);
        assert!(pos[&k(2)].0 < pos[&k(3)].0);
        assert!(pos[&k(3)].0 < pos[&k(4)].0);
    }

    #[test]
    fn test_crossing_avoidance() {
        let nodes = vec![
            mk_node(1, "SrcA", 0, 1),
            mk_node(2, "SrcB", 0, 1),
            mk_node(3, "DstTop", 1, 0),
            mk_node(4, "DstBot", 1, 0),
        ];
        let k = |id| NodeKey { group_id: id, split_part: 0 };
        // Crossed initial order: SrcA -> DstBot, SrcB -> DstTop
        let edges = vec![
            LayoutEdge { src_key: k(1), src_port_id: 101, dst_key: k(4), dst_port_id: 1 },
            LayoutEdge { src_key: k(2), src_port_id: 101, dst_key: k(3), dst_port_id: 1 },
        ];
        let pos = compute_layout(&nodes, &edges, None, true);
        // Either (SrcA, DstBot) are both above (SrcB, DstTop) or both below -> no crossing!
        let dy_src = pos[&k(1)].1 - pos[&k(2)].1;
        let dy_dst = pos[&k(4)].1 - pos[&k(3)].1;
        assert!(dy_src * dy_dst > 0);
    }

    #[test]
    fn test_skip_edge_clears_intermediate_box() {
        let nodes = vec![
            mk_node(1, "Src", 0, 2),
            mk_node(2, "Mid1", 2, 2),
            mk_node(3, "Mid2", 2, 2),
            mk_node(4, "Sink", 2, 0),
        ];
        let k = |id| NodeKey { group_id: id, split_part: 0 };
        let edges = vec![
            LayoutEdge { src_key: k(1), src_port_id: 101, dst_key: k(2), dst_port_id: 1 },
            LayoutEdge { src_key: k(2), src_port_id: 101, dst_key: k(3), dst_port_id: 1 },
            LayoutEdge { src_key: k(3), src_port_id: 101, dst_key: k(4), dst_port_id: 1 },
            // Skip edge from Src directly to Sink
            LayoutEdge { src_key: k(1), src_port_id: 102, dst_key: k(4), dst_port_id: 2 },
        ];
        let pos = compute_layout(&nodes, &edges, Some((0.0, 0.0, 3100.0, 2400.0)), true);
        let x1 = pos[&k(1)].0 as f64 + nodes[0].width;
        let y1 = pos[&k(1)].1 as f64 + nodes[0].out_port_y[&102];
        let x2 = pos[&k(4)].0 as f64;
        let y2 = pos[&k(4)].1 as f64 + nodes[3].in_port_y[&2];

        for &mid_idx in &[1usize, 2usize] {
            let n = &nodes[mid_idx];
            let (bx, by) = pos[&n.key];
            if let Some((ey_min, ey_max)) =
                edge_y_range_over_x(x1, y1, x2, y2, bx as f64, bx as f64 + n.width, true)
            {
                assert!(
                    ey_max < by as f64 || ey_min > by as f64 + n.height,
                    "Skip edge intersects intermediate box {}",
                    n.name
                );
            }
        }
    }

    #[test]
    fn test_cycle_and_isolated_nodes() {
        let nodes = vec![
            mk_node(1, "LoopA", 1, 1),
            mk_node(2, "LoopB", 1, 1),
            mk_node(3, "Isolated", 1, 1),
        ];
        let k = |id| NodeKey { group_id: id, split_part: 0 };
        let edges = vec![
            LayoutEdge { src_key: k(1), src_port_id: 101, dst_key: k(2), dst_port_id: 1 },
            LayoutEdge { src_key: k(2), src_port_id: 101, dst_key: k(1), dst_port_id: 1 },
        ];
        let pos = compute_layout(&nodes, &edges, None, true);
        assert_eq!(pos.len(), 3);
        // Isolated node sits below the connected component
        assert!(pos[&k(3)].1 > pos[&k(1)].1.max(pos[&k(2)].1));
    }
}
