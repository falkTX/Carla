import * as Comlink from 'comlink';
import { useEffect, useState, useCallback, useRef } from 'react';
import { type OscWorkerType } from '../osc-worker';
import { useQueryClient } from '@tanstack/react-query';
import { type PluginInfo, type PortCounts, type ProgramCounts, type InternalParams, type PluginParameter, type PatchbayConnection } from '../components/CarlaInfoDisplay';

interface ReceivedOSCMessage { address: string; args: any[]; }
const worker = new Worker(new URL('../osc-worker.ts', import.meta.url), { type: 'module' });
const oscWorker = Comlink.wrap<OscWorkerType>(worker);

interface PendingMutation {
  type: string;
  pluginId?: number;
  paramId?: number;
  originalValue?: any;
  connectionId?: string;
}
const pendingMutations = new Map<number, PendingMutation>();

export interface OSCMessageArgs { address: string; args: any[]; }
export interface UseOsc {
  connect: (host: string, tcpPort: number, udpPort: number) => Promise<void>;
  disconnect: () => void;
  sendMessage: (address: string, ...args: any[]) => void;
  addPlugin: (pluginIdentifier: string) => void;
  removePlugin: (pluginId: number) => void;
  connectPorts: (sPlugId: number, sPortIdx: number, tPlugId: number, tPortIdx: number) => void;
  disconnectPorts: (connectionId: string) => void;
  setParameterValue: (pluginId: number, paramId: number, value: number) => void;
  connectionStatus: string;
  lastMessage: OSCMessageArgs | null;
}

let nextMessageId = 1;
const OSC_TARGET_NAME = 'Carla';

export const useOsc = (): UseOsc => {
  const [connectionStatus, setConnectionStatus] = useState<string>('Idle');
  const [lastMessage, setLastMessage] = useState<OSCMessageArgs | null>(null);
  const queryClient = useQueryClient();
  const statusCallbackRef = useRef(setConnectionStatus);

  const paramUpdate = useCallback((pluginId: number, paramId: number, updates: Partial<PluginParameter>) => {
    queryClient.setQueryData<PluginParameter[]>(['pluginParameters', pluginId], (oldParams = []) => {
      const paramIndex = oldParams.findIndex(p => p.id === paramId);
      if (paramIndex !== -1) {
        const newParams = [...oldParams];
        newParams[paramIndex] = { ...newParams[paramIndex], ...updates };
        return newParams;
      }
      return [...oldParams, { id: paramId, name: `Param ${paramId}`, ...updates }];
    });
  }, [queryClient]);

  const messageCallbackRef = useRef((msg: ReceivedOSCMessage) => {
    setLastMessage({ address: msg.address, args: msg.args });
    console.log('Message received in useOsc:', msg.address, msg.args);

    if (msg.address === '/ctrl/info') {
      const [pluginId, type_, category, hints, uniqueId, optsAvail, optsEnabled, name, filename, iconName, realName, label, maker, copyright] = msg.args;
      const newPluginData: PluginInfo = { id: pluginId, type: type_, category, hints, uniqueId, optionsAvailable: optsAvail, optionsEnabled: optsEnabled, name, filename, iconName, realName, label, maker, copyright };
      queryClient.setQueryData<PluginInfo[]>(['pluginList'], (oldData = []) => {
        const existingPluginIndex = oldData.findIndex(p => p.id === pluginId);
        if (existingPluginIndex !== -1) { const updatedData = [...oldData]; updatedData[existingPluginIndex] = { ...updatedData[existingPluginIndex], ...newPluginData }; return updatedData; }
        return [...oldData, newPluginData];
      });
    }
    if (msg.address === '/ctrl/ports') {
      const [pluginId, audioIns, audioOuts, midiIns, midiOuts, paramIns, paramOuts, paramTotal] = msg.args;
      const portData: PortCounts = { audioIns, audioOuts, midiIns, midiOuts, cvIns: paramIns, cvOuts: paramOuts, paramTotal };
      queryClient.setQueryData<PluginInfo[]>(['pluginList'], (oldData = []) => oldData.map(p => p.id === pluginId ? { ...p, ports: portData } : p));
    }
    if (msg.address === '/ctrl/pcount') {
      const [pluginId, pcount, mpcount] = msg.args;
      const programCountsData: ProgramCounts = { programs: pcount, midiPrograms: mpcount };
      queryClient.setQueryData<PluginInfo[]>(['pluginList'], (oldData = []) => oldData.map(p => p.id === pluginId ? { ...p, programCounts: programCountsData } : p));
    }
    if (msg.address === '/ctrl/iparams') {
      const [pluginId, active, drywet, volume, balLeft, balRight, pan, ctrlChan] = msg.args;
      const internalParamsData: InternalParams = { active: Boolean(active), dryWet: drywet, volume, balanceLeft: balLeft, balanceRight: balRight, panning: pan, ctrlChannel: ctrlChan };
      queryClient.setQueryData<PluginInfo[]>(['pluginList'], (oldData = []) => oldData.map(p => p.id === pluginId ? { ...p, internalParams: internalParamsData } : p));
    }

    if (msg.address === '/ctrl/paramInfo') { const [pluginId, paramId, name, unit, comment, groupName] = msg.args; paramUpdate(pluginId, paramId, { name, unit, comment, groupName }); }
    if (msg.address === '/ctrl/paramData') { const [pluginId, paramId, type, hints, midiChannel, mappedControlIndex, mappedMinimum, mappedMaximum, value] = msg.args; paramUpdate(pluginId, paramId, { type, hints, midiChannel, mappedControlIndex, mappedMinimum, mappedMaximum, value }); }
    if (msg.address === '/ctrl/paramRanges') { const [pluginId, paramId, defaultValue, minimum, maximum, step, stepSmall, stepLarge] = msg.args; paramUpdate(pluginId, paramId, { defaultValue, minimum, maximum, step, stepSmall, stepLarge }); }
    if (msg.address === '/ctrl/param') { const [pluginId, paramId, value] = msg.args; paramUpdate(pluginId, paramId, { value }); }

    if (msg.address === '/ctrl/cb') {
      const [action, value1, _value2, _value3, _valuef, valueStr] = msg.args;
      if (action === 2 /* PLUGIN_REMOVE */) {
            queryClient.setQueryData<PluginInfo[]>(['pluginList'], (oldData = []) => oldData.filter(p => p.id !== value1));
            queryClient.removeQueries({ queryKey: ['pluginParameters', value1], exact: true });
      }
      if (action === 3 /* PORTS_CONNECT */) {
        const connId = valueStr;
        const parts = connId.match(/(\d+):(\d+)>(\d+):(\d+)/);
        if (parts) { const newConn: PatchbayConnection = { id: connId, sourcePluginId: parseInt(parts[1]), sourcePortIndex: parseInt(parts[2]), targetPluginId: parseInt(parts[3]), targetPortIndex: parseInt(parts[4]) };
          queryClient.setQueryData<PatchbayConnection[]>(['patchbayConnections'], (old = []) => old.find(c=>c.id===connId)?old:[...old, newConn]);}
      }
      if (action === 4 /* PORTS_DISCONNECT */) {
        queryClient.setQueryData<PatchbayConnection[]>(['patchbayConnections'], (old = []) => old.filter(c => c.id !== String(value1)));
      }
    }

    if (msg.address === '/ctrl/resp') {
      const [messageId, errorStr] = msg.args;
      const mutationDetails = pendingMutations.get(messageId);
      if (mutationDetails) {
        if (errorStr && errorStr.length > 0) {
          console.error(`Mutation Error (ID: ${messageId}, Type: ${mutationDetails.type}): ${errorStr}`); // Corrected
          if (mutationDetails.type === 'setParameterValue' && mutationDetails.pluginId && mutationDetails.paramId && mutationDetails.originalValue !== undefined) {
            paramUpdate(mutationDetails.pluginId, mutationDetails.paramId, { value: mutationDetails.originalValue });
          }
        } else {
          console.log(`Mutation Success (ID: ${messageId}, Type: ${mutationDetails.type})`); // Corrected
        }
        pendingMutations.delete(messageId);
      } else {
        console.warn(`Received /ctrl/resp for unknown messageId: ${messageId}`); // Corrected
      }
    }
  });

  useEffect(() => { statusCallbackRef.current = setConnectionStatus; }, [setConnectionStatus]);
  useEffect(() => {
    oscWorker.onStatusChange(Comlink.proxy(statusCallbackRef.current));
    oscWorker.onMessage(Comlink.proxy(messageCallbackRef.current));
    return () => { /* Cleanup */ };
  }, [messageCallbackRef]);

  const connect = useCallback(async (host: string, tcpPort: number, udpPort: number) => {
    try { await oscWorker.connect(host, tcpPort, udpPort); } catch (e) { setConnectionStatus(`Failed: ${(e as Error).message}`); } // Corrected
  }, []);
  const disconnect = useCallback(() => { oscWorker.disconnect(); }, []);
  const sendMessage = useCallback((address: string, ...args: any[]) => { oscWorker.sendMessage(address, ...args); }, []);

  const addPlugin = useCallback((pluginIdentifier: string) => {
    const mid = nextMessageId++; pendingMutations.set(mid, {type: 'addPlugin'});
    oscWorker.sendMessage('/ctrl/add_plugin', mid, -1, '', '', pluginIdentifier, '', pluginIdentifier);
  }, []);

  const removePlugin = useCallback((pluginId: number) => {
    const mid = nextMessageId++;
    const oldPlugins = queryClient.getQueryData<PluginInfo[]>(['pluginList']) || [];
    const pluginToRemove = oldPlugins.find(p => p.id === pluginId);
    pendingMutations.set(mid, {type: 'removePlugin', pluginId, originalValue: pluginToRemove });
    queryClient.setQueryData<PluginInfo[]>(['pluginList'], oldPlugins.filter(p => p.id !== pluginId));
    queryClient.removeQueries({queryKey:['pluginParameters',pluginId],exact:true});
    oscWorker.sendMessage('/ctrl/remove_plugin', mid, pluginId);
  }, [queryClient]);

  const connectPorts = useCallback((sPlugId: number, sPortIdx: number, tPlugId: number, tPortIdx: number) => {
    const mid = nextMessageId++; pendingMutations.set(mid, {type: 'connectPorts'});
    oscWorker.sendMessage('/ctrl/patchbay_connect', mid, sPlugId, sPortIdx, tPlugId, tPortIdx);
  }, []);

  const disconnectPorts = useCallback((connectionId: string) => {
    const mid = nextMessageId++;
    const oldConns = queryClient.getQueryData<PatchbayConnection[]>(['patchbayConnections']) || [];
    pendingMutations.set(mid, {type: 'disconnectPorts', connectionId, originalValue: oldConns.find(c => c.id === connectionId)});
    queryClient.setQueryData<PatchbayConnection[]>(['patchbayConnections'], oldConns.filter(c => c.id !== connectionId));
    oscWorker.sendMessage('/ctrl/patchbay_disconnect', mid, connectionId);
  }, [queryClient]);

  const setParameterValue = useCallback((pluginId: number, paramId: number, value: number) => {
    const mid = nextMessageId++;
    const params = queryClient.getQueryData<PluginParameter[]>(['pluginParameters', pluginId]) || [];
    const originalParam = params.find(p => p.id === paramId);
    pendingMutations.set(mid, { type: 'setParameterValue', pluginId, paramId, originalValue: originalParam?.value });

    paramUpdate(pluginId, paramId, { value }); // Optimistic update

    const path = `/${OSC_TARGET_NAME}/${pluginId}/set_parameter_value`;
    oscWorker.sendMessage(path, mid, paramId, value);
  }, [queryClient, paramUpdate]);

  return { connect, disconnect, sendMessage, addPlugin, removePlugin, connectPorts, disconnectPorts, setParameterValue, connectionStatus, lastMessage };
};
