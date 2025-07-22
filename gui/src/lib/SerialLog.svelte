<script>
	import { serialLog, serialManager } from './serial.js';

	let { visible = $bindable(false) } = $props();

	function clearLog() {
		serialManager.clearLog();
	}

	function getLogClass(type) {
		switch (type) {
			case 'error': return 'log-error';
			case 'send': return 'log-send';
			case 'receive': return 'log-receive';
			default: return 'log-info';
		}
	}
</script>

{#if visible}
	<div class="log-overlay" onclick={() => visible = false} role="button" tabindex="0" onkeydown={(e) => e.key === 'Escape' && (visible = false)}>
		<div class="log-modal" onclick={(e) => e.stopPropagation()} onkeydown={(e) => e.key === 'Escape' && (visible = false)} role="dialog" tabindex="-1">
			<div class="log-header">
				<h2>Serial Communication Log</h2>
				<div class="log-controls">
					<button onclick={clearLog}>Clear Log</button>
					<button onclick={() => visible = false}>Close</button>
				</div>
			</div>
			
			<div class="log-content">
				{#each $serialLog as entry}
					<div class="log-entry {getLogClass(entry.type)}">
						<span class="log-time">{entry.timestamp}</span>
						<span class="log-message">{entry.message}</span>
					</div>
				{:else}
					<div class="log-empty">No log entries yet</div>
				{/each}
			</div>
		</div>
	</div>
{/if}

<style>
	.log-overlay {
		position: fixed;
		top: 0;
		left: 0;
		right: 0;
		bottom: 0;
		background: rgba(0, 0, 0, 0.5);
		display: flex;
		justify-content: center;
		align-items: center;
		z-index: 1000;
	}

	.log-modal {
		background: white;
		border-radius: 8px;
		width: 90%;
		max-width: 800px;
		height: 80%;
		display: flex;
		flex-direction: column;
		box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
	}

	.log-header {
		padding: 20px;
		border-bottom: 1px solid #dee2e6;
		display: flex;
		justify-content: space-between;
		align-items: center;
	}

	.log-header h2 {
		margin: 0;
		color: #333;
	}

	.log-controls {
		display: flex;
		gap: 8px;
	}

	.log-controls button {
		padding: 8px 16px;
		border: 1px solid #ced4da;
		border-radius: 4px;
		background: white;
		cursor: pointer;
		font-size: 0.9em;
	}

	.log-controls button:hover {
		background: #f8f9fa;
	}

	.log-content {
		flex: 1;
		overflow-y: auto;
		padding: 16px;
		font-family: 'Courier New', monospace;
		font-size: 0.85em;
	}

	.log-entry {
		display: flex;
		margin-bottom: 4px;
		padding: 4px 0;
	}

	.log-time {
		min-width: 100px;
		color: #6c757d;
		margin-right: 12px;
	}

	.log-message {
		flex: 1;
		word-break: break-all;
	}

	.log-info .log-message {
		color: #495057;
	}

	.log-error .log-message {
		color: #dc3545;
		font-weight: 500;
	}

	.log-send .log-message {
		color: #007bff;
	}

	.log-receive .log-message {
		color: #28a745;
	}

	.log-empty {
		text-align: center;
		color: #6c757d;
		font-style: italic;
		margin-top: 40px;
	}
</style>