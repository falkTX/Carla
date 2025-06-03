import React, { useState, useEffect } from 'react';
import type { ChangeEvent } from 'react'; // For verbatimModuleSyntax
import { useQuery } from '@tanstack/react-query'; // Removed useQueryClient

// === Type Definitions ===
export interface PluginParameter { /* ... existing ... */
  id: number; name: string; unit?: string; comment?: string; groupName?: string;
  type?: number; hints?: number; midiChannel?: number; mappedControlIndex?: number;
  mappedMinimum?: number; mappedMaximum?: number; defaultValue?: number; minimum?: number;
  maximum?: number; step?: number; stepSmall?: number; stepLarge?: number; value?: number;
}
export interface PortCounts { /* ... existing ... */
  audioIns: number; audioOuts: number; midiIns: number; midiOuts: number;
  cvIns?: number; cvOuts?: number; paramTotal?: number;
}
export interface ProgramCounts { /* ... existing ... */ programs: number; midiPrograms: number; }
export interface InternalParams { /* ... existing ... */
  active: boolean; dryWet: number; volume: number; balanceLeft: number;
  balanceRight: number; panning: number; ctrlChannel: number;
}
export interface PluginInfo { /* ... existing ... */
  id: number; type?: number; category?: string; hints?: number; uniqueId?: number;
  optionsAvailable?: number; optionsEnabled?: number; name: string; filename?: string;
  iconName?: string; realName?: string; label?: string; maker?: string; copyright?: string;
  ports?: PortCounts; programCounts?: ProgramCounts; internalParams?: InternalParams;
}
export interface EngineInfo { [key: string]: any; }
export interface PatchbayConnection { /* ... existing ... */
  id: string; sourcePluginId: number; sourcePortIndex: number;
  targetPluginId: number; targetPortIndex: number;
  sourcePortName?: string; targetPortName?: string;
}

// === Components ===
interface CarlaInfoDisplayProps {
  removePlugin: (pluginId: number) => void;
  connectPortsRequest: (sPlugId: number, sPortIdx: number, tPlugId: number, tPortIdx: number) => void;
  disconnectPortsRequest: (connId: string) => void;
  setParameterValueRequest: (pluginId: number, paramId: number, value: number) => void;
}

const ParameterInput: React.FC<{ pluginId: number, param: PluginParameter, setParameterValueRequest: Function }> = ({ pluginId, param, setParameterValueRequest }) => {
  const [localValue, setLocalValue] = useState(param.value?.toString() ?? '');

  useEffect(() => {
    setLocalValue(param.value?.toString() ?? '');
  }, [param.value]);

  const handleChange = (e: ChangeEvent<HTMLInputElement>) => {
    setLocalValue(e.target.value);
  };

  const handleBlur = () => {
    const numValue = parseFloat(localValue);
    if (!isNaN(numValue) && numValue !== param.value) {
      setParameterValueRequest(pluginId, param.id, numValue);
    }
  };

  const handleKeyPress = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      handleBlur();
      (e.target as HTMLInputElement).blur(); // Remove focus
    }
  };

  // Basic number input, could be a slider if min/max/step are known
  return (
    <input
      type='number'
      value={localValue}
      onChange={handleChange}
      onBlur={handleBlur}
      onKeyPress={handleKeyPress}
      step={param.stepSmall || param.step || 0.01} // Provide a sensible default step
      min={param.minimum}
      max={param.maximum}
      title={`Min: ${param.minimum ?? 'N/A'}, Max: ${param.maximum ?? 'N/A'}, Default: ${param.defaultValue ?? 'N/A'}`}
      className='input input-bordered input-xs w-24 ml-2'
    />
  );
};

const PluginParamsDisplay: React.FC<{ pluginId: number, setParameterValueRequest: Function }> = ({ pluginId, setParameterValueRequest }) => {
  const { data: parameters, isLoading } = useQuery<PluginParameter[], Error>({
    queryKey: ['pluginParameters', pluginId],
  });

  if (isLoading) return <p className='text-xs text-gray-500'>Loading parameters...</p>;
  if (!parameters || parameters.length === 0) return <p className='text-xs text-gray-500'>No parameters.</p>;

  return (
    <div className='mt-1 pl-4 border-l-2 border-gray-200'>
      <h4 className='text-xs font-semibold text-gray-700'>Parameters:</h4>
      <ul className='space-y-1 text-xs'>
        {parameters.map(param => (
          <li key={param.id} title={`ID: ${param.id}, Unit: ${param.unit || 'N/A'}`} className='flex items-center justify-between'>
            <span>
              {param.groupName && <span className='text-gray-500'>[${param.groupName}] </span>}
              {param.name}: {param.value?.toFixed(4) ?? 'N/A'}
            </span>
            <ParameterInput pluginId={pluginId} param={param} setParameterValueRequest={setParameterValueRequest} />
          </li>
        ))}
      </ul>
    </div>
  );
};

const PatchbayConnectionsDisplay: React.FC<{ disconnectPortsRequest: (connId: string) => void }> = ({ disconnectPortsRequest }) => { /* ... existing ... */
  const { data: connections } = useQuery<PatchbayConnection[], Error>({ queryKey: ['patchbayConnections'] });
  if (!connections || connections.length === 0) return <p className='text-xs text-gray-500'>No patchbay connections.</p>;
  return (
    <div className='mt-3'>
      <h3 className='font-medium mb-1'>Connections ({connections.length}):</h3>
      <ul className='space-y-1 text-xs'>
        {connections.map(conn => (
          <li key={conn.id} className='flex justify-between items-center p-1 bg-gray-100 rounded'>
            <span>(ID: {conn.id}) Plg {conn.sourcePluginId}:{conn.sourcePortIndex} &rarr; Plg {conn.targetPluginId}:{conn.targetPortIndex}</span>
            <button onClick={() => disconnectPortsRequest(conn.id)} className='btn btn-warning btn-xs'>X</button>
          </li>
        ))}
      </ul>
    </div>
  );
};

export const CarlaInfoDisplay: React.FC<CarlaInfoDisplayProps> = ({ removePlugin, disconnectPortsRequest, setParameterValueRequest }) => {
  const { data: engineInfo, error: engineInfoError } = useQuery<EngineInfo, Error>({ queryKey: ['engineInfo'] });
  const { data: pluginList } = useQuery<PluginInfo[], Error>({ queryKey: ['pluginList'] });
  const [expandedPluginId, setExpandedPluginId] = React.useState<number | null>(null);

  const togglePluginDetails = (pluginId: number) => setExpandedPluginId(expandedPluginId === pluginId ? null : pluginId);

  return (
    <div className='mt-4 p-4 border rounded-md bg-gray-50'>
      <h2 className='text-xl font-semibold mb-2'>Carla Host Information</h2>
      {engineInfo && <div className='mb-3'><h3 className='font-medium'>Engine Info:</h3><pre className='text-sm bg-white p-2 rounded overflow-x-auto'>{JSON.stringify(engineInfo, null, 2)}</pre></div>}
      {engineInfoError && <p>Error loading engine info: {engineInfoError.message}</p>}
      {pluginList && pluginList.length > 0 ? (
        <div className='mb-3'>
          <h3 className='font-medium mb-1'>Plugins ({pluginList.length}):</h3>
          <ul className='space-y-2'>
            {pluginList.map((plugin) => (
              <li key={plugin.id} className='text-sm border p-3 rounded bg-white shadow-sm'>
                <div className='flex justify-between items-center'>
                  <div>
                    <strong onClick={() => togglePluginDetails(plugin.id)} className='cursor-pointer hover:text-blue-600'>{plugin.name}</strong> (ID: {plugin.id})
                    {plugin.label && <span className='text-xs'>, Label: {plugin.label}</span>}
                  </div>
                  <button onClick={() => removePlugin(plugin.id)} className='btn btn-error btn-xs'>Remove</button>
                </div>
                {expandedPluginId === plugin.id && <PluginParamsDisplay pluginId={plugin.id} setParameterValueRequest={setParameterValueRequest} />}
              </li>
            ))}
          </ul>
        </div>
      ) : (
        <p className='text-xs text-gray-500'>No plugins loaded.</p>
      )}
      <PatchbayConnectionsDisplay disconnectPortsRequest={disconnectPortsRequest} />
    </div>
  );
};
