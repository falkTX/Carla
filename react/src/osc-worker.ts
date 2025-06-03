import OSC from 'osc-js';
import { expose } from 'comlink';

let oscOverTCP: OSC | null = null;
let oscOverUDP: OSC | null = null; // Placeholder for UDP-like communication

let statusCallback: ((status: string) => void) | null = null;
let messageCallback: ((message: OSC.Message) => void) | null = null;

const workerMethods = {
  connect: async (host: string, tcpPort: number, udpPort: number) => {
    console.log(`[Worker] Attempting to connect to ${host}, TCP: ${tcpPort}, UDP (conceptual): ${udpPort}`);
    statusCallback?.('Connecting...');

    // Close existing connections if any
    if (oscOverTCP) {
      oscOverTCP.close();
      oscOverTCP = null;
    }
    // Placeholder for UDP, actual implementation will depend on server capabilities for WebSocket
    if (oscOverUDP) {
      oscOverUDP.close();
      oscOverUDP = null;
    }

    // TCP Connection
    // Note: Browsers use WebSockets for OSC. The server (Carla) must have a WebSocket OSC interface.
    // The standard 'osc-js' WebsocketClientPlugin connects to a WebSocket server that then bridges to OSC.
    // If Carla doesn't directly support WebSocket to OSC, a bridge server (like a small Node.js app) might be needed.
    // For now, assuming Carla or an intermediary bridge provides a WebSocket endpoint for OSC messages.
    oscOverTCP = new OSC({
      plugin: new OSC.WebsocketClientPlugin({ host, port: tcpPort, secure: false })
    });

    oscOverTCP.on('open', () => {
      console.log('[Worker] OSC TCP (WebSocket) connection opened.');
      statusCallback?.('OSC TCP Connected');
      // Send a registration message, similar to how the Python client does.
      // The path /register and the format of the client URL might need to be exact.
      // Example: oscOverTCP?.send(new OSC.Message('/register', `osc.tcp://${someClientIdentifier}:${someClientPort}/CarlaCtrlPWA`));
      // For now, we are not sending any specific registration message until server expectation is clear.
    });

    oscOverTCP.on('close', () => {
      console.log('[Worker] OSC TCP (WebSocket) connection closed.');
      statusCallback?.('OSC TCP Disconnected');
    });

    oscOverTCP.on('error', (err: Error) => {
      console.error('[Worker] OSC TCP (WebSocket) Error:', err);
      statusCallback?.(`OSC TCP Error: ${err.message}`);
    });

    oscOverTCP.on('*', (message: OSC.Message) => {
      console.log('[Worker] OSC TCP (WebSocket) Message Received:', message.address, message.args);
      messageCallback?.(message);
    });

    try {
      oscOverTCP.open(); // This is asynchronous for WebSocket connections
    } catch (error) {
      console.error('[Worker] Failed to initiate OSC TCP (WebSocket) connection:', error);
      statusCallback?.(`OSC TCP Connection Failed: ${(error as Error).message}`);
      return;
    }

    // UDP Communication Placeholder:
    // Browsers cannot directly create UDP sockets.
    // Options:
    // 1. Send all messages (including those traditionally over UDP) via the single TCP/WebSocket connection.
    //    Carla's OSC server would need to handle this. This is the most likely scenario for a pure web PWA.
    // 2. Use a separate WebSocket connection to a different port/path on the server if the server
    //    is designed to treat that as a 'UDP-like' channel (e.g., for high-frequency, non-critical messages).
    // 3. A server-side bridge that accepts WebSockets and forwards to Carla via UDP.
    // For this initial implementation, we will assume all messages go through oscOverTCP.
    // The 'sendDataMessage' method will just use oscOverTCP.
    console.log('[Worker] UDP communication is conceptual. All messages will be sent via the primary TCP/WebSocket connection.');
    statusCallback?.('UDP conceptual (using TCP WebSocket)');
  },

  sendMessage: (address: string, ...args: any[]) => {
    if (oscOverTCP && oscOverTCP.status() === OSC.STATUS.IS_OPEN) {
      console.log(`[Worker] Sending OSC (via TCP WebSocket): ${address}`, args);
      try {
        oscOverTCP.send(new OSC.Message(address, ...args));
      } catch (error) {
        console.error(`[Worker] Error sending OSC message ${address} via TCP WebSocket:`, error);
        statusCallback?.(`Error sending message: ${(error as Error).message}`);
      }
    } else {
      console.warn('[Worker] OSC TCP (WebSocket) connection not open. Message not sent:', address, args);
      statusCallback?.('Error: OSC TCP not connected. Cannot send message.');
    }
  },

  // This function is kept for conceptual separation, but it will use the TCP WebSocket.
  sendDataMessage: (address: string, ...args: any[]) => {
    if (oscOverTCP && oscOverTCP.status() === OSC.STATUS.IS_OPEN) {
      console.log(`[Worker] Sending Data OSC (via TCP WebSocket): ${address}`, args);
      try {
        oscOverTCP.send(new OSC.Message(address, ...args));
      } catch (error) {
        console.error(`[Worker] Error sending Data OSC message ${address} via TCP WebSocket:`, error);
        statusCallback?.(`Error sending data message: ${(error as Error).message}`);
      }
    } else {
      console.warn('[Worker] OSC TCP (WebSocket) connection not open for data message. Message not sent:', address, args);
      statusCallback?.('Error: OSC TCP not connected. Cannot send data message.');
    }
  },

  onStatusChange: (callback: (status: string) => void) => {
    statusCallback = callback;
  },

  onMessage: (callback: (message: OSC.Message) => void) => {
    messageCallback = callback;
  },

  disconnect: () => {
    if (oscOverTCP) {
      oscOverTCP.close(); // This is asynchronous
      oscOverTCP = null;
    }
    // oscOverUDP cleanup if it were used
    console.log('[Worker] OSC connections commanded to close.');
    statusCallback?.('Disconnected');
  }
};

expose(workerMethods);

// This export is crucial for Comlink in the main thread to get type inference
export type OscWorkerType = typeof workerMethods;
