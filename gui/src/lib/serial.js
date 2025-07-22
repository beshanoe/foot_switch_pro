import { writable } from 'svelte/store';

// Keep using stores for cross-component reactive state
export const serialPort = writable(null);
export const isConnected = writable(false);
export const serialLog = writable([]);

class SerialManager {
	constructor() {
		this.port = null;
		this.reader = null;
		this.writer = null;
		this.readBuffer = '';
	}

	async connect() {
		try {
			if (!('serial' in navigator)) {
				throw new Error('WebSerial API not supported in this browser');
			}

			this.port = await navigator.serial.requestPort();
			await this.port.open({ baudRate: 9600 });

			this.reader = this.port.readable.getReader();
			this.writer = this.port.writable.getWriter();

			this.startReading();

			// Verify device identity
			await this.verifyDevice();

			serialPort.set(this.port);
			isConnected.set(true);
			this.addLog('Connected to foot_switch_pro device', 'info');

		} catch (error) {
			this.addLog(`Connection failed: ${error.message}`, 'error');
			if (this.port) {
				try {
					await this.disconnect();
				} catch (disconnectError) {
					// Ignore disconnect errors during failed connection
				}
			}
			throw error;
		}
	}

	async verifyDevice() {
		return new Promise((resolve, reject) => {
			const timeout = setTimeout(() => {
				this.removeEventListener('deviceInfo', handler);
				reject(new Error('Device verification timeout - not a foot_switch_pro device'));
			}, 3000);

			const handler = (event) => {
				clearTimeout(timeout);
				if (event.detail === 'foot_switch_pro-v1.0') {
					resolve();
				} else {
					reject(new Error(`Unknown device: ${event.detail}`));
				}
			};

			this.addEventListener('deviceInfo', handler);
			this.sendCommand('INFO');
		});
	}

	async disconnect() {
		try {
			if (this.reader) {
				await this.reader.cancel();
				await this.reader.releaseLock();
			}
			if (this.writer) {
				await this.writer.releaseLock();
			}
			if (this.port) {
				await this.port.close();
			}

			this.port = null;
			this.reader = null;
			this.writer = null;

			serialPort.set(null);
			isConnected.set(false);
			this.addLog('Disconnected from device', 'info');

		} catch (error) {
			this.addLog(`Disconnect failed: ${error.message}`, 'error');
			throw error;
		}
	}

	async startReading() {
		try {
			while (this.reader) {
				const { value, done } = await this.reader.read();
				if (done) break;

				const text = new TextDecoder().decode(value);
				this.readBuffer += text;

				// Process complete lines
				let lines = this.readBuffer.split('\n');
				this.readBuffer = lines.pop(); // Keep incomplete line in buffer

				for (let line of lines) {
					line = line.trim();
					if (line) {
						this.addLog(`← ${line}`, 'receive');
						this.handleIncomingData(line);
					}
				}
			}
		} catch (error) {
			if (error.name !== 'NetworkError') {
				this.addLog(`Read error: ${error.message}`, 'error');
			}
		}
	}

	async sendCommand(command) {
		if (!this.writer) {
			throw new Error('Not connected to device');
		}

		try {
			const data = new TextEncoder().encode(command + '\n');
			await this.writer.write(data);
			this.addLog(`→ ${command}`, 'send');
		} catch (error) {
			this.addLog(`Send error: ${error.message}`, 'error');
			throw error;
		}
	}

	handleIncomingData(data) {
		// Handle device responses
		if (data.startsWith('SWITCH ')) {
			// Parse switch configuration response
			const match = data.match(/SWITCH (\d+): (.+)/);
			if (match) {
				const switchNum = parseInt(match[1]);
				const config = match[2];
				this.parseSwitchConfig(switchNum, config);
			}
		} else if (data.startsWith('BANK ')) {
			// Parse bank name response
			const match = data.match(/BANK (\d+): (.+)/);
			if (match) {
				const bankIndex = parseInt(match[1]);
				const bankName = match[2];
				window.dispatchEvent(new CustomEvent('bankName', {
					detail: { bankIndex, bankName }
				}));
			}
		} else if (data === 'foot_switch_pro-v1.0') {
			// Device identification response
			window.dispatchEvent(new CustomEvent('deviceInfo', {
				detail: data
			}));
		}
	}

	addEventListener(type, handler) {
		window.addEventListener(type, handler);
	}

	removeEventListener(type, handler) {
		window.removeEventListener(type, handler);
	}

	parseSwitchConfig(switchNum, config) {
		let action = { type: 'NONE' };

		if (config.startsWith('NOTE ')) {
			action = {
				type: 'NOTE',
				value: parseInt(config.substring(5))
			};
		} else if (config.startsWith('CC ')) {
			const parts = config.substring(3).split(' ');
			action = {
				type: 'CC',
				ccNum: parseInt(parts[0]),
				ccVal: parseInt(parts[1])
			};
		} else if (config.startsWith('KEY ')) {
			action = {
				type: 'KEY',
				value: config.substring(4) // Now contains human-readable format like "CTRL+A"
			};
		} else if (config.startsWith('BANK_SEL ')) {
			action = {
				type: 'BANK_SEL',
				bankIndex: parseInt(config.substring(9))
			};
		} else if (config === 'BANK_NEXT') {
			action = { type: 'BANK_NEXT' };
		} else if (config === 'BANK_PREV') {
			action = { type: 'BANK_PREV' };
		} else if (config === 'NONE') {
			action = { type: 'NONE' };
		}

		// Dispatch custom event with switch configuration
		window.dispatchEvent(new CustomEvent('switchConfig', {
			detail: { switchNum, action }
		}));
	}

	addLog(message, type = 'info') {
		const timestamp = new Date().toLocaleTimeString();
		serialLog.update(logs => [
			...logs,
			{ timestamp, message, type }
		]);
	}

	clearLog() {
		serialLog.set([]);
	}
}

export const serialManager = new SerialManager();