import React, { useState } from 'react';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { ReactQueryDevtools } from '@tanstack/react-query-devtools';
import { useOsc } from './services/useOsc';

const queryClient = new QueryClient();

function App() {
  const { connect, disconnect, sendMessage, connectionStatus, lastMessage } = useOsc();
  const [host, setHost] = useState('127.0.0.1');
  // Default Carla OSC TCP port for remote control is often 22752, but can be configured.
  // The Python client uses CARLA_DEFAULT_OSC_TCP_PORT_NUMBER which might be defined elsewhere.
  // For now, let's use a common default or make it configurable.
  const [tcpPort, setTcpPort] = useState('22752');
  const [udpPort, setUdpPort] = useState('22752'); // Conceptual, as worker uses TCP for all

  const handleConnect = () => {
    connect(host, parseInt(tcpPort, 10), parseInt(udpPort, 10));
  };

  const handleSendTestMessage = () => {
    // Example: Requesting engine status (path might need adjustment based on Carla's OSC API)
    sendMessage('/ctrl/engine_status_req'); // Replace with actual path
    sendMessage('/Carla/ping'); // A common OSC ping path, might need adjustment
  };

  return (
    <div className='container mx-auto p-4'>
      <h1 className='text-2xl font-bold mb-4'>Carla Control PWA</h1>
      <div className='space-y-2 mb-4'>
        <div>
          <label htmlFor='host' className='block text-sm font-medium text-gray-700'>Host:</label>
          <input type='text' id='host' value={host} onChange={(e) => setHost(e.target.value)} className='mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm' />
        </div>
        <div>
          <label htmlFor='tcpPort' className='block text-sm font-medium text-gray-700'>TCP Port:</label>
          <input type='text' id='tcpPort' value={tcpPort} onChange={(e) => setTcpPort(e.target.value)} className='mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm' />
        </div>
        {/* UDP Port input can be kept for conceptual completeness or removed if not actively used for distinct connections */}
        <div>
          <label htmlFor='udpPort' className='block text-sm font-medium text-gray-700'>UDP Port (Conceptual):</label>
          <input type='text' id='udpPort' value={udpPort} onChange={(e) => setUdpPort(e.target.value)} className='mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm' />
        </div>
      </div>
      <div className='space-x-2 mb-4'>
        <button onClick={handleConnect} className='px-4 py-2 bg-blue-500 text-white rounded hover:bg-blue-700'>Connect</button>
        <button onClick={() => disconnect()} className='px-4 py-2 bg-red-500 text-white rounded hover:bg-red-700'>Disconnect</button>
        <button onClick={handleSendTestMessage} className='px-4 py-2 bg-green-500 text-white rounded hover:bg-green-700'>Send Test Message</button>
      </div>
      <p className='mb-2'>Status: <span className='font-semibold'>{connectionStatus}</span></p>
      {lastMessage && (
        <div className='p-2 bg-gray-100 rounded'>
          <p className='font-semibold'>Last Message:</p>
          <p>Address: {lastMessage.address}</p>
          <p>Args: {JSON.stringify(lastMessage.args)}</p>
        </div>
      )}
    </div>
  );
}

function RootApp() {
  return (
    <QueryClientProvider client={queryClient}>
      <App />
      <ReactQueryDevtools initialIsOpen={false} />
    </QueryClientProvider>
  )
}

export default RootApp;
