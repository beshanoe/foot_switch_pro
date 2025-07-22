import { writable } from 'svelte/store';

// Keep using store for now as runes don't replace stores for cross-component state
export const toasts = writable([]);

let toastId = 0;

export function addToast(message, type = 'info', duration = 3000) {
	const id = toastId++;
	const toast = { id, message, type, duration };
	
	toasts.update(current => [...current, toast]);
	
	if (duration > 0) {
		setTimeout(() => {
			removeToast(id);
		}, duration);
	}
	
	return id;
}

export function removeToast(id) {
	toasts.update(current => current.filter(toast => toast.id !== id));
}

// Convenience functions
export const success = (message, duration) => addToast(message, 'success', duration);
export const error = (message, duration) => addToast(message, 'error', duration);
export const warning = (message, duration) => addToast(message, 'warning', duration);
export const info = (message, duration) => addToast(message, 'info', duration);