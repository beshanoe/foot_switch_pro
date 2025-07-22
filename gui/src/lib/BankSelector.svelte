<script>
	import { serialManager } from './serial.js';
	import { success, error } from './toast.js';

	let { currentBank = $bindable(0), bankNames = $bindable(Array(16).fill('')) } = $props();

	let isChangingBank = $state(false);
	let editingBankName = $state(false);
	let tempBankName = $state('');

	async function selectBank(bankIndex) {
		if (isChangingBank) return;
		
		isChangingBank = true;
		try {
			// Always send SEL_BANK command to device
			await serialManager.sendCommand(`SEL_BANK ${bankIndex}`);
			currentBank = bankIndex;
			success(`Switched to Bank ${bankIndex + 1}`);
			
			// Dispatch event to parent to reload bank data
			window.dispatchEvent(new CustomEvent('bankChanged', {
				detail: { bankIndex }
			}));
		} catch (error) {
			console.error('Failed to select bank:', error);
			error(`Failed to switch to Bank ${bankIndex + 1}: ${error.message}`);
		}
		isChangingBank = false;
	}

	async function nextBank() {
		const nextIndex = (currentBank + 1) % 16;
		await selectBank(nextIndex);
	}

	async function prevBank() {
		const prevIndex = (currentBank + 15) % 16;
		await selectBank(prevIndex);
	}

	function startEditingBankName() {
		tempBankName = bankNames[currentBank] || '';
		editingBankName = true;
	}

	function cancelEditingBankName() {
		editingBankName = false;
		tempBankName = '';
	}

	async function saveBankName() {
		try {
			await serialManager.sendCommand(`NAME_BANK ${currentBank} ${tempBankName}`);
			bankNames[currentBank] = tempBankName;
			bankNames = [...bankNames]; // Trigger reactivity
			success(`Bank ${currentBank + 1} renamed to "${tempBankName}"`);
			editingBankName = false;
			tempBankName = '';
		} catch (error) {
			console.error('Failed to save bank name:', error);
			error(`Failed to rename bank: ${error.message}`);
		}
	}

	async function clearCurrentBank() {
		if (!confirm(`Are you sure you want to clear all switches in ${getBankDisplayName(currentBank)}?`)) {
			return;
		}

		try {
			await serialManager.sendCommand(`CLEAR_BANK ${currentBank}`);
			success(`${getBankDisplayName(currentBank)} cleared successfully`);
			
			// Dispatch event to reload bank data
			window.dispatchEvent(new CustomEvent('bankChanged', {
				detail: { bankIndex: currentBank }
			}));
		} catch (error) {
			console.error('Failed to clear bank:', error);
			error(`Failed to clear ${getBankDisplayName(currentBank)}: ${error.message}`);
		}
	}

	function getBankDisplayName(index) {
		return bankNames[index] || `Bank ${index}`;
	}

	function handleKeydown(event) {
		if (event.key === 'Enter') {
			saveBankName();
		} else if (event.key === 'Escape') {
			cancelEditingBankName();
		}
	}
</script>

<div class="bank-selector">
	<div class="bank-controls">
		<button 
			class="bank-nav-btn" 
			onclick={prevBank}
			disabled={isChangingBank}
			title="Previous Bank"
		>
			◀
		</button>

		<div class="bank-info">
			<div class="bank-dropdown">
				<select 
					bind:value={currentBank} 
					onchange={(e) => selectBank(parseInt(e.target.value))}
					disabled={isChangingBank}
				>
					{#each Array(16) as _, i}
						<option value={i}>{getBankDisplayName(i)}</option>
					{/each}
				</select>
			</div>

			<div class="bank-name">
				{#if editingBankName}
					<input 
						type="text" 
						bind:value={tempBankName}
						placeholder="Bank name (max 16 chars)"
						maxlength="16"
						onkeydown={handleKeydown}
						class="bank-name-input"
					/>
					<div class="name-edit-buttons">
						<button class="btn-small btn-primary" onclick={saveBankName}>✓</button>
						<button class="btn-small btn-secondary" onclick={cancelEditingBankName}>✗</button>
					</div>
				{:else}
					<span class="current-bank-name">{getBankDisplayName(currentBank)}</span>
					<button 
						class="edit-name-btn" 
						onclick={startEditingBankName}
						title="Edit bank name"
					>
						✏️
					</button>
				{/if}
			</div>
		</div>

		<button 
			class="bank-nav-btn" 
			onclick={nextBank}
			disabled={isChangingBank}
			title="Next Bank"
		>
			▶
		</button>
	</div>

	<div class="bank-actions">
		<button 
			class="btn-small btn-danger" 
			onclick={clearCurrentBank}
			title="Clear all switches in current bank"
		>
			Clear Bank
		</button>
	</div>
</div>

<style>
	.bank-selector {
		background: white;
		border-radius: 8px;
		padding: 20px;
		box-shadow: 0 2px 10px rgba(0,0,0,0.1);
		margin-bottom: 20px;
	}

	.bank-controls {
		display: flex;
		align-items: center;
		gap: 16px;
		margin-bottom: 16px;
	}

	.bank-nav-btn {
		width: 40px;
		height: 40px;
		border: 2px solid #007bff;
		border-radius: 50%;
		background: white;
		color: #007bff;
		cursor: pointer;
		font-size: 1.2em;
		display: flex;
		align-items: center;
		justify-content: center;
		transition: all 0.2s ease;
	}

	.bank-nav-btn:hover:not(:disabled) {
		background: #007bff;
		color: white;
	}

	.bank-nav-btn:disabled {
		opacity: 0.5;
		cursor: not-allowed;
	}

	.bank-info {
		flex: 1;
		display: flex;
		flex-direction: column;
		gap: 8px;
	}

	.bank-dropdown select {
		width: 100%;
		padding: 8px 12px;
		border: 1px solid #ced4da;
		border-radius: 4px;
		font-size: 1rem;
		font-weight: 500;
	}

	.bank-name {
		display: flex;
		align-items: center;
		gap: 8px;
	}

	.current-bank-name {
		font-size: 1.1em;
		color: #333;
		font-weight: 500;
	}

	.edit-name-btn {
		background: none;
		border: none;
		cursor: pointer;
		padding: 4px;
		border-radius: 4px;
		font-size: 1em;
	}

	.edit-name-btn:hover {
		background: #f8f9fa;
	}

	.bank-name-input {
		flex: 1;
		padding: 6px 10px;
		border: 1px solid #007bff;
		border-radius: 4px;
		font-size: 1rem;
	}

	.name-edit-buttons {
		display: flex;
		gap: 4px;
	}

	.bank-actions {
		display: flex;
		gap: 8px;
		justify-content: center;
	}

	.btn-small {
		padding: 6px 12px;
		border: none;
		border-radius: 4px;
		cursor: pointer;
		font-size: 0.85em;
		font-weight: 500;
	}

	.btn-primary {
		background: #28a745;
		color: white;
	}

	.btn-secondary {
		background: #6c757d;
		color: white;
	}

	.btn-danger {
		background: #dc3545;
		color: white;
	}

	.btn-small:hover {
		opacity: 0.9;
	}

	@media (max-width: 768px) {
		.bank-controls {
			flex-direction: column;
			gap: 12px;
		}

		.bank-nav-btn {
			width: 36px;
			height: 36px;
		}
	}
</style>