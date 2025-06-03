import { useState, useEffect } from 'react'; // Corrected: Removed 'React,'
import { QueryClientProvider, QueryClient } from '@tanstack/react-query'; // Ensure QueryClient is imported
import { ReactQueryDevtools } from '@tanstack/react-query-devtools';
import { useOsc } from './services/useOsc';
import { CarlaInfoDisplay } from './components/CarlaInfoDisplay';

const queryClient = new QueryClient();

function App() {
  const {
    connect, disconnect, sendMessage,
    addPlugin, removePlugin,
    connectPorts, disconnectPorts,
    setParameterValue,
    connectionStatus, lastMessage
  } = useOsc();

  const [host, setHost] = useState('127.0.0.1');
  const [tcpPort, setTcpPort] = useState('22752');
  const [pluginIdText, setPluginIdText] = useState('mda Delay');
  const [srcPlugId, setSrcPlugId] = useState('');
  const [srcPortIdx, setSrcPortIdx] = useState('');
  const [tgtPlugId, setTgtPlugId] = useState('');
  const [tgtPortIdx, setTgtPortIdx] = useState('');

  const handleConnect = () => connect(host, parseInt(tcpPort, 10), 0);
  useEffect(() => { if (connectionStatus === 'OSC TCP Connected') sendMessage('/ctrl/patchbay_refresh'); }, [connectionStatus, sendMessage]);
  const handleAddPlugin = () => { if (pluginIdText.trim()) addPlugin(pluginIdText.trim()); else alert('Plugin ID needed'); };
  const handleConnectPorts = () => {
    if (srcPlugId && srcPortIdx && tgtPlugId && tgtPortIdx) {
      connectPorts(parseInt(srcPlugId), parseInt(srcPortIdx), parseInt(tgtPlugId), parseInt(tgtPortIdx));
    } else { alert('All connection fields required.'); }
  };

  return (
    <QueryClientProvider client={queryClient}>
      <div className='container mx-auto p-4'>
        <h1 className='text-2xl font-bold mb-4'>Carla Control PWA</h1>
        {/* Connection UI */}
        <div className='space-y-4 mb-4 p-4 border rounded-lg shadow bg-white'>
          <h2 className='text-lg font-semibold'>Connection</h2>
          <div className='grid grid-cols-1 md:grid-cols-3 gap-4 items-end'>
            <div><label>Host:</label><input type='text' value={host} onChange={(e) => setHost(e.target.value)} className='mt-1 block w-full input input-bordered input-sm' /></div>
            <div><label>TCP Port:</label><input type='text' value={tcpPort} onChange={(e) => setTcpPort(e.target.value)} className='mt-1 block w-full input input-bordered input-sm' /></div>
            <div className='flex space-x-2 pt-4'><button onClick={handleConnect} className='btn btn-primary btn-sm w-full'>Connect</button><button onClick={disconnect} className='btn btn-secondary btn-sm w-full'>Disconnect</button></div>
          </div>
          <p>Status: {connectionStatus}</p>
        </div>
        {/* Add Plugin UI */}
        <div className='space-y-2 mb-4 p-4 border rounded-lg shadow bg-white'>
          <h2 className='text-lg font-semibold'>Add Plugin</h2>
          <div className='flex items-end gap-2'>
            <div className='flex-grow'><label>Plugin ID:</label><input type='text' value={pluginIdText} onChange={(e) => setPluginIdText(e.target.value)} className='mt-1 block w-full input input-bordered input-sm' /></div>
            <button onClick={handleAddPlugin} className='btn btn-accent btn-sm'>Add Plugin</button>
          </div>
        </div>
        {/* Test Connect Ports UI */}
        <div className='space-y-2 mb-4 p-4 border rounded-lg shadow bg-white'>
          <h2 className='text-lg font-semibold'>Test Connect Ports</h2>
          <div className='grid grid-cols-2 md:grid-cols-5 gap-2 items-end'>
            <div><label className='text-xs'>SrcPlgID:</label><input type='text' value={srcPlugId} onChange={e=>setSrcPlugId(e.target.value)} className='input input-bordered input-xs w-full'/></div>
            <div><label className='text-xs'>SrcPrtIdx:</label><input type='text' value={srcPortIdx} onChange={e=>setSrcPortIdx(e.target.value)} className='input input-bordered input-xs w-full'/></div>
            <div><label className='text-xs'>TgtPlgID:</label><input type='text' value={tgtPlugId} onChange={e=>setTgtPlugId(e.target.value)} className='input input-bordered input-xs w-full'/></div>
            <div><label className='text-xs'>TgtPrtIdx:</label><input type='text' value={tgtPortIdx} onChange={e=>setTgtPortIdx(e.target.value)} className='input input-bordered input-xs w-full'/></div>
            <button onClick={handleConnectPorts} className='btn btn-info btn-sm'>Connect Ports</button>
          </div>
        </div>

        {lastMessage && <div className='mt-2 p-2 bg-gray-100 rounded text-xs border'><p>Last OSC: {lastMessage.address} {JSON.stringify(lastMessage.args)}</p></div>}
        <CarlaInfoDisplay
          removePlugin={removePlugin}
          connectPortsRequest={connectPorts}
          disconnectPortsRequest={disconnectPorts}
          setParameterValueRequest={setParameterValue}
        />
      </div>
      <ReactQueryDevtools initialIsOpen={false} />
    </QueryClientProvider>
  );
}

export default App;
