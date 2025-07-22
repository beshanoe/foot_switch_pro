<script>
	import { toasts, removeToast } from './toast.js';
	
	function getIcon(type) {
		switch (type) {
			case 'success': return '✓';
			case 'error': return '✕';
			case 'warning': return '⚠';
			default: return 'ℹ';
		}
	}
</script>

<div class="toast-container">
	{#each $toasts as toast (toast.id)}
		<div class="toast toast-{toast.type}" role="alert">
			<span class="toast-icon">{getIcon(toast.type)}</span>
			<span class="toast-message">{toast.message}</span>
			<button class="toast-close" onclick={() => removeToast(toast.id)} aria-label="Close">×</button>
		</div>
	{/each}
</div>

<style>
	.toast-container {
		position: fixed;
		top: 20px;
		right: 20px;
		z-index: 1000;
		display: flex;
		flex-direction: column;
		gap: 10px;
		pointer-events: none;
	}
	
	.toast {
		display: flex;
		align-items: center;
		gap: 10px;
		padding: 12px 16px;
		border-radius: 6px;
		box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
		font-size: 14px;
		max-width: 400px;
		pointer-events: auto;
		animation: slideIn 0.3s ease-out;
	}
	
	.toast-success {
		background: #d4edda;
		color: #155724;
		border-left: 4px solid #28a745;
	}
	
	.toast-error {
		background: #f8d7da;
		color: #721c24;
		border-left: 4px solid #dc3545;
	}
	
	.toast-warning {
		background: #fff3cd;
		color: #856404;
		border-left: 4px solid #ffc107;
	}
	
	.toast-info {
		background: #d1ecf1;
		color: #0c5460;
		border-left: 4px solid #17a2b8;
	}
	
	.toast-icon {
		font-weight: bold;
		font-size: 16px;
	}
	
	.toast-message {
		flex: 1;
	}
	
	.toast-close {
		background: none;
		border: none;
		font-size: 18px;
		cursor: pointer;
		opacity: 0.7;
		padding: 0;
		width: 20px;
		height: 20px;
		display: flex;
		align-items: center;
		justify-content: center;
	}
	
	.toast-close:hover {
		opacity: 1;
	}
	
	@keyframes slideIn {
		from {
			transform: translateX(100%);
			opacity: 0;
		}
		to {
			transform: translateX(0);
			opacity: 1;
		}
	}
</style>