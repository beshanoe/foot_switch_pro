<script>
	import { onMount } from 'svelte';
	import { isConnected, serialManager } from '../lib/serial.js';
	import SwitchCell from '../lib/SwitchCell.svelte';
	import SerialLog from '../lib/SerialLog.svelte';
	import BankSelector from '../lib/BankSelector.svelte';
	import Toast from '../lib/Toast.svelte';
	import { success, error, warning, info } from '../lib/toast.js';

	let switchActions = $state(Array(6).fill({ type: 'NONE' }));
	let showLog = $state(false);
	let connectionStatus = $state('disconnected');
	let isReading = $state(false);
	let currentBank = $state(0);
	let bankNames = $state(Array(16).fill(''));

	onMount(() => {
		// Listen for switch configuration updates
		window.addEventListener('switchConfig', (event) => {
			const { switchNum, action } = event.detail;
			switchActions[switchNum] = { ...action }; // Create new object to trigger reactivity
			switchActions = [...switchActions]; // Trigger reactivity for array
		});

		// Listen for bank name updates
		window.addEventListener('bankName', (event) => {
			const { bankIndex, bankName } = event.detail;
			bankNames[bankIndex] = bankName;
			bankNames = [...bankNames]; // Trigger reactivity
		});

		// Listen for bank changes
		window.addEventListener('bankChanged', (event) => {
			const { bankIndex } = event.detail;
			currentBank = bankIndex;
			readSettings(); // Reload switch settings for new bank
		});

		// Subscribe to connection status
		$effect(() => {
			const unsubscribe = isConnected.subscribe(connected => {
				connectionStatus = connected ? 'connected' : 'disconnected';
			});
			return unsubscribe;
		});
	});

	async function connectToDevice() {
		try {
			await serialManager.connect();
			success('Successfully connected to foot_switch_pro device!');
			// Auto-read settings and bank names after connection
			setTimeout(() => {
				readSettings();
				loadBankNames();
			}, 1000);
		} catch (error) {
			console.error('Connection failed:', error);
			error(`Connection failed: ${error.message}`);
		}
	}

	async function disconnectFromDevice() {
		try {
			await serialManager.disconnect();
			info('Disconnected from device');
		} catch (error) {
			console.error('Disconnection failed:', error);
			error(`Disconnection failed: ${error.message}`);
		}
	}

	async function readSettings() {
		if (connectionStatus !== 'connected') return;
		
		isReading = true;
		try {
			await serialManager.sendCommand(`GET ALL ${currentBank}`);
			success('Settings loaded successfully');
		} catch (error) {
			console.error('Failed to read settings:', error);
			error(`Failed to read settings: ${error.message}`);
		} finally {
			setTimeout(() => isReading = false, 2000);
		}
	}

	async function loadBankNames() {
		if (connectionStatus !== 'connected') return;
		
		try {
			await serialManager.sendCommand('NAMES');
		} catch (error) {
			console.error('Failed to load bank names:', error);
			error(`Failed to load bank names: ${error.message}`);
		}
	}

	async function clearAllBanks() {
		if (connectionStatus !== 'connected') return;
		
		if (!confirm('Are you sure you want to clear ALL banks? This will erase all switch configurations!')) {
			return;
		}

		try {
			for (let i = 0; i < 16; i++) {
				await serialManager.sendCommand(`CLEAR_BANK ${i}`);
			}
			// Reset local state
			switchActions = Array(6).fill({ type: 'NONE' });
			bankNames = Array(16).fill('');
			success('All banks cleared successfully');
			// Re-read current bank settings
			setTimeout(() => readSettings(), 1000);
		} catch (error) {
			console.error('Failed to clear all banks:', error);
			error(`Failed to clear all banks: ${error.message}`);
		}
	}

	function toggleLog() {
		showLog = !showLog;
	}
</script>

<svelte:head>
	<title>foot_switch_pro Configuration</title>
</svelte:head>

<div class="app">
	<header class="header">
		<h1>foot_switch_pro Configuration</h1>
		<div class="connection-status status-{connectionStatus}">
			{connectionStatus === 'connected' ? '🟢' : '🔴'} 
			{connectionStatus.toUpperCase()}
		</div>
	</header>

	<div class="toolbar">
		{#if connectionStatus === 'disconnected'}
			<button class="btn btn-primary" onclick={connectToDevice}>
				Connect Device
			</button>
		{:else}
			<button class="btn btn-secondary" onclick={disconnectFromDevice}>
				Disconnect
			</button>
		{/if}

		<button 
			class="btn btn-secondary" 
			onclick={readSettings}
			disabled={connectionStatus !== 'connected' || isReading}
		>
			{isReading ? 'Reading...' : 'Read Settings'}
		</button>

		<button 
			class="btn btn-danger" 
			onclick={clearAllBanks}
			disabled={connectionStatus !== 'connected'}
		>
			Clear All Banks
		</button>

		<button class="btn btn-secondary" onclick={toggleLog}>
			Show Log
		</button>
	</div>

	{#if connectionStatus === 'disconnected'}
		<div class="connection-notice">
			<h2>Connect to your foot_switch_pro device</h2>
			<p>Make sure your device is connected via USB and click "Connect Device"</p>
			<p><strong>Note:</strong> WebSerial API requires a secure connection (HTTPS) and is supported in Chrome, Edge, and other Chromium-based browsers.</p>
		</div>
	{:else}
		<BankSelector 
			bind:currentBank={currentBank}
			bind:bankNames={bankNames}
		/>

		<div class="switches-container">
			<h2>Footswitch Configuration</h2>
			<div class="switches-row">
				{#each switchActions as action, index}
					<SwitchCell 
						switchNum={index} 
						bind:action={switchActions[index]}
						currentBank={currentBank}
					/>
				{/each}
			</div>
		</div>
	{/if}

	<SerialLog bind:visible={showLog} />
	<Toast />
</div>

<style>
	:global(body) {
		margin: 0;
		padding: 0;
		font-family: system-ui, -apple-system, sans-serif;
		background: #f8f9fa;
	}

	.app {
		min-height: 100vh;
		padding: 20px;
	}

	.header {
		display: flex;
		justify-content: space-between;
		align-items: center;
		margin-bottom: 30px;
		padding-bottom: 20px;
		border-bottom: 2px solid #dee2e6;
	}

	.header h1 {
		margin: 0;
		color: #333;
		font-size: 2rem;
	}

	.connection-status {
		font-weight: bold;
		padding: 8px 16px;
		border-radius: 20px;
		font-size: 0.9rem;
	}

	.status-connected {
		background: #d4edda;
		color: #155724;
		border: 1px solid #c3e6cb;
	}

	.status-disconnected {
		background: #f8d7da;
		color: #721c24;
		border: 1px solid #f5c6cb;
	}

	.toolbar {
		display: flex;
		gap: 12px;
		margin-bottom: 30px;
		flex-wrap: wrap;
	}

	.btn {
		padding: 10px 20px;
		border: none;
		border-radius: 6px;
		cursor: pointer;
		font-size: 0.95rem;
		font-weight: 500;
		transition: all 0.2s ease;
	}

	.btn:disabled {
		opacity: 0.6;
		cursor: not-allowed;
	}

	.btn-primary {
		background: #007bff;
		color: white;
	}

	.btn-primary:hover:not(:disabled) {
		background: #0056b3;
	}

	.btn-secondary {
		background: #6c757d;
		color: white;
	}

	.btn-secondary:hover:not(:disabled) {
		background: #545b62;
	}

	.btn-danger {
		background: #dc3545;
		color: white;
	}

	.btn-danger:hover:not(:disabled) {
		background: #c82333;
	}

	.connection-notice {
		text-align: center;
		padding: 60px 20px;
		background: white;
		border-radius: 8px;
		box-shadow: 0 2px 10px rgba(0,0,0,0.1);
		max-width: 600px;
		margin: 0 auto;
	}

	.connection-notice h2 {
		color: #333;
		margin-bottom: 16px;
	}

	.connection-notice p {
		color: #666;
		line-height: 1.6;
		margin-bottom: 12px;
	}

	.switches-container {
		background: white;
		border-radius: 8px;
		padding: 30px;
		box-shadow: 0 2px 10px rgba(0,0,0,0.1);
	}

	.switches-container h2 {
		margin: 0 0 24px 0;
		color: #333;
		text-align: center;
		font-size: 1.5rem;
	}

	.switches-row {
		display: flex;
		gap: 16px;
		justify-content: center;
		flex-wrap: wrap;
		margin: 0 auto;
	}

	@media (max-width: 768px) {
		.header {
			flex-direction: column;
			align-items: flex-start;
			gap: 16px;
		}

		.switches-row {
			flex-direction: column;
		}

		.toolbar {
			justify-content: center;
		}
	}
</style>