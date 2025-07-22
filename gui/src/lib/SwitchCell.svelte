<script>
	import { serialManager } from './serial.js';
	import { success, error } from './toast.js';

	let { switchNum = 0, action = $bindable({ type: 'NONE' }), currentBank = 0 } = $props();

	// Internal state for form inputs
	let currentActionType = $state(action.type);
	let noteValue = $state(action.value || 60);
	let ccNum = $state(action.ccNum || 1);
	let ccVal = $state(action.ccVal || 127);
	let keyModifier = $state('');
	let keyValue = $state('A');
	let bankSelIndex = $state(0);

	const actionTypes = [
		{ value: 'NONE', label: 'None' },
		{ value: 'NOTE', label: 'MIDI Note' },
		{ value: 'CC', label: 'MIDI CC' },
		{ value: 'KEY', label: 'Keyboard' },
		{ value: 'BANK_SEL', label: 'Select Bank' },
		{ value: 'BANK_NEXT', label: 'Next Bank' },
		{ value: 'BANK_PREV', label: 'Previous Bank' }
	];

	const keyOptions = [
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
		'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'F1', 'F2', 'F3', 'F4', 'F5', 'F6', 'F7', 'F8', 'F9', 'F10', 'F11', 'F12',
		'ENTER', 'ESC', 'TAB', 'SPACE', 'BACKSPACE', 'DELETE', 'INSERT',
		'HOME', 'END', 'PAGEUP', 'PAGEDOWN', 'UP', 'DOWN', 'LEFT', 'RIGHT'
	];

	const modifierOptions = ['', 'CTRL', 'SHIFT', 'ALT', 'CTRL+SHIFT', 'CTRL+ALT', 'SHIFT+ALT'];

	// Update internal state when action prop changes
	$effect(() => {
		currentActionType = action.type;
		if (action.type === 'NOTE') {
			noteValue = action.value || 60;
		} else if (action.type === 'CC') {
			ccNum = action.ccNum || 1;
			ccVal = action.ccVal || 127;
		} else if (action.type === 'KEY') {
			// Parse keyboard action from device response (now in human-readable format)
			if (action.value) {
				// New format is human-readable like "CTRL+A"
				const parts = action.value.split('+');
				if (parts.length > 1) {
					keyModifier = parts.slice(0, -1).join('+');
					keyValue = parts[parts.length - 1];
				} else {
					keyModifier = '';
					keyValue = action.value;
				}
			}
		} else if (action.type === 'BANK_SEL') {
			bankSelIndex = action.bankIndex || 0;
		}
	});

	let isSaving = $state(false);
	let saveStatus = $state('idle'); // 'idle', 'saving', 'success', 'error'

	// Track changes
	let hasChanges = $derived(() => {
		const hasTypeChange = currentActionType !== action.type;
		const hasNoteChange = currentActionType === 'NOTE' && noteValue !== action.value;
		const hasCCChange = currentActionType === 'CC' && (ccNum !== action.ccNum || ccVal !== action.ccVal);
		const hasKeyChange = currentActionType === 'KEY' && (
			keyModifier !== extractModifier(action.value) || 
			keyValue !== extractKey(action.value)
		);
		const hasBankChange = currentActionType === 'BANK_SEL' && bankSelIndex !== action.bankIndex;
		
		return hasTypeChange || hasNoteChange || hasCCChange || hasKeyChange || hasBankChange;
	});

	function extractModifier(keyStr) {
		if (!keyStr || typeof keyStr !== 'string') return '';
		if (keyStr.includes('+')) {
			const parts = keyStr.split('+');
			return parts.slice(0, -1).join('+');
		}
		return '';
	}

	function extractKey(keyStr) {
		if (!keyStr || typeof keyStr !== 'string') return 'A';
		if (keyStr.includes('+')) {
			const parts = keyStr.split('+');
			return parts[parts.length - 1];
		}
		return keyStr || 'A';
	}

	async function saveAction() {
		if (!hasChanges() || isSaving) return;
		
		isSaving = true;
		saveStatus = 'saving';
		let command = '';
		
		switch (currentActionType) {
			case 'NOTE':
				command = `SET ${currentBank} ${switchNum} NOTE ${noteValue}`;
				break;
			case 'CC':
				command = `SET ${currentBank} ${switchNum} CC ${ccNum} ${ccVal}`;
				break;
			case 'KEY':
				const keyStr = keyModifier ? `${keyModifier}+${keyValue}` : keyValue;
				command = `SET ${currentBank} ${switchNum} KEY ${keyStr}`;
				break;
			case 'BANK_SEL':
				command = `SET ${currentBank} ${switchNum} BANK_SEL ${bankSelIndex}`;
				break;
			case 'BANK_NEXT':
				command = `SET ${currentBank} ${switchNum} BANK_NEXT`;
				break;
			case 'BANK_PREV':
				command = `SET ${currentBank} ${switchNum} BANK_PREV`;
				break;
			case 'NONE':
				command = `SET ${currentBank} ${switchNum} NONE`;
				break;
		}

		if (command) {
			try {
				await serialManager.sendCommand(command);
				
				// Update action prop to reflect saved state
				if (currentActionType === 'NOTE') {
					action = { type: 'NOTE', value: noteValue };
				} else if (currentActionType === 'CC') {
					action = { type: 'CC', ccNum, ccVal };
				} else if (currentActionType === 'KEY') {
					const keyStr = keyModifier ? `${keyModifier}+${keyValue}` : keyValue;
					action = { type: 'KEY', value: keyStr };
				} else if (currentActionType === 'BANK_SEL') {
					action = { type: 'BANK_SEL', bankIndex: bankSelIndex };
				} else if (currentActionType === 'BANK_NEXT') {
					action = { type: 'BANK_NEXT' };
				} else if (currentActionType === 'BANK_PREV') {
					action = { type: 'BANK_PREV' };
				} else {
					action = { type: 'NONE' };
				}
				
				saveStatus = 'success';
				success(`Switch ${switchNum + 1} saved successfully`);
				
				// Show success state for 2 seconds
				setTimeout(() => {
					saveStatus = 'idle';
				}, 2000);
				
			} catch (error) {
				console.error('Failed to save action:', error);
				error(`Failed to save Switch ${switchNum + 1}: ${error.message}`);
				saveStatus = 'error';
				
				// Show error state for 3 seconds
				setTimeout(() => {
					saveStatus = 'idle';
				}, 3000);
			}
		}
		
		isSaving = false;
	}

	// Reactive display for actions
	let actionDisplay = $derived(getActionDisplay(action));

	function getActionDisplay(action) {
		switch (action.type) {
			case 'NOTE':
				return `Note ${action.value}`;
			case 'CC':
				return `CC ${action.ccNum}:${action.ccVal}`;
			case 'KEY':
				return action.value; // Now human-readable
			case 'BANK_SEL':
				return `Bank ${action.bankIndex}`;
			case 'BANK_NEXT':
				return 'Next Bank';
			case 'BANK_PREV':
				return 'Previous Bank';
			default:
				return 'None';
		}
	}
</script>

<div class="switch-cell">
	<div class="switch-header">
		<h3>Switch {switchNum}</h3>
		<div class="current-action">
			{actionDisplay}
		</div>
	</div>

	<div class="action-config">
		<label>
			Action Type:
			<select bind:value={currentActionType}>
				{#each actionTypes as actionType}
					<option value={actionType.value}>{actionType.label}</option>
				{/each}
			</select>
		</label>

		{#if currentActionType === 'NOTE'}
			<label>
				MIDI Note (0-127):
				<input
					type="number"
					min="0"
					max="127"
					bind:value={noteValue}
				/>
			</label>
		{:else if currentActionType === 'CC'}
			<label>
				CC Number (0-127):
				<input
					type="number"
					min="0"
					max="127"
					bind:value={ccNum}
				/>
			</label>
			<label>
				CC Value (0-127):
				<input
					type="number"
					min="0"
					max="127"
					bind:value={ccVal}
				/>
			</label>
		{:else if currentActionType === 'KEY'}
			<label>
				Modifier:
				<select bind:value={keyModifier}>
					{#each modifierOptions as modifier}
						<option value={modifier}>{modifier || 'None'}</option>
					{/each}
				</select>
			</label>
			<label>
				Key:
				<select bind:value={keyValue}>
					{#each keyOptions as key}
						<option value={key}>{key}</option>
					{/each}
				</select>
			</label>
		{:else if currentActionType === 'BANK_SEL'}
			<label>
				Target Bank (0-15):
				<input
					type="number"
					min="0"
					max="15"
					bind:value={bankSelIndex}
				/>
			</label>
		{/if}

		<button 
			class="save-btn" 
			class:has-changes={hasChanges() && saveStatus === 'idle'}
			class:saving={saveStatus === 'saving'}
			class:success={saveStatus === 'success'}
			class:error={saveStatus === 'error'}
			disabled={(!hasChanges() && saveStatus === 'idle') || isSaving}
			onclick={saveAction}
		>
			{#if saveStatus === 'saving'}
				Saving...
			{:else if saveStatus === 'success'}
				✓ Saved
			{:else if saveStatus === 'error'}
				✗ Error
			{:else if hasChanges()}
				Save Changes
			{:else}
				No Changes
			{/if}
		</button>
	</div>
</div>

<style>
	.switch-cell {
		border: 2px solid #333;
		border-radius: 8px;
		padding: 16px;
		margin: 8px;
		background: #f8f9fa;
		min-width: 200px;
		flex: 1;
	}

	.switch-header {
		margin-bottom: 16px;
		text-align: center;
	}

	.switch-header h3 {
		margin: 0 0 8px 0;
		color: #333;
		font-size: 1.2em;
	}

	.current-action {
		background: #e9ecef;
		padding: 4px 8px;
		border-radius: 4px;
		font-size: 0.9em;
		color: #495057;
	}

	.action-config {
		display: flex;
		flex-direction: column;
		gap: 12px;
	}

	label {
		display: flex;
		flex-direction: column;
		font-size: 0.9em;
		font-weight: 500;
		color: #495057;
	}

	select, input {
		margin-top: 4px;
		padding: 6px;
		border: 1px solid #ced4da;
		border-radius: 4px;
		font-size: 0.9em;
	}

	select:focus, input:focus {
		outline: none;
		border-color: #007bff;
		box-shadow: 0 0 0 2px rgba(0, 123, 255, 0.25);
	}

	.save-btn {
		padding: 8px 16px;
		border: 1px solid #ced4da;
		border-radius: 4px;
		background: #f8f9fa;
		color: #495057;
		cursor: pointer;
		font-size: 0.9em;
		margin-top: 8px;
		transition: all 0.2s ease;
	}

	.save-btn:disabled {
		opacity: 0.6;
		cursor: not-allowed;
	}

	.save-btn.has-changes {
		background: #007bff;
		color: white;
		border-color: #0056b3;
	}

	.save-btn.has-changes:hover:not(:disabled) {
		background: #0056b3;
		border-color: #004085;
	}

	.save-btn.saving {
		background: #ffc107;
		color: #212529;
		border-color: #e0a800;
	}

	.save-btn.success {
		background: #28a745;
		color: white;
		border-color: #1e7e34;
	}

	.save-btn.error {
		background: #dc3545;
		color: white;
		border-color: #bd2130;
	}

	.save-btn.success:hover,
	.save-btn.error:hover {
		opacity: 0.9;
	}
</style>