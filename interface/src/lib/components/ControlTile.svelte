<script lang="ts">
	type ControlKind = 'door' | 'light';
	type ControlTone = 'rose' | 'sage';

	type Props = {
		kind: ControlKind;
		label: string;
		caption: string;
		tone: ControlTone;
		pressed: boolean;
		disabled?: boolean;
		ariaLabel: string;
		onToggle: () => void;
		onHold?: () => void;
		holdDurationMs?: number;
	};

	let {
		kind,
		label,
		caption,
		tone,
		pressed,
		disabled = false,
		ariaLabel,
		onToggle,
		onHold,
		holdDurationMs = 5000
	}: Props = $props();

	let holdTimer: ReturnType<typeof setTimeout> | null = null;
	let holdTriggered = false;

	function clearHoldTimer() {
		if (holdTimer) {
			clearTimeout(holdTimer);
			holdTimer = null;
		}
	}

	function startHold() {
		if (!onHold || disabled) {
			return;
		}

		holdTriggered = false;
		clearHoldTimer();
		holdTimer = setTimeout(() => {
			holdTriggered = true;
			onHold();
			clearHoldTimer();
		}, holdDurationMs);
	}

	function stopHold() {
		clearHoldTimer();
	}

	function handleClick() {
		if (holdTriggered) {
			holdTriggered = false;
			return;
		}

		onToggle();
	}

	const baseClass = [
		'cursor-pointer rounded-[18px] border-0 px-6 py-6 text-center [webkit-tap-highlight-color:transparent]',
		'flex min-h-[320px] flex-col items-center justify-center gap-4',
		'transition-transform duration-180 ease-out hover:-translate-y-0.5 active:translate-y-0 active:scale-[0.985]',
		'disabled:cursor-not-allowed disabled:opacity-70 disabled:hover:translate-y-0 disabled:active:translate-y-0 disabled:active:scale-100',
		'focus-visible:outline focus-visible:outline-3 focus-visible:outline-offset-4 focus-visible:outline-[rgba(60,43,32,0.18)]',
		'max-[959px]:min-h-[200px] max-[959px]:gap-4'
	].join(' ');

	const toneClass = $derived(
		tone === 'rose'
			? 'bg-[var(--rose)] text-[var(--rose-strong)] shadow-[0_4px_12px_rgba(122,79,93,0.08),inset_0_1px_0_rgba(255,255,255,0.28)]'
			: 'bg-[var(--sage)] text-[var(--sage-strong)] shadow-[0_4px_12px_rgba(68,99,68,0.08),inset_0_1px_0_rgba(255,255,255,0.28)]'
	);

	const captionClass = $derived(
		tone === 'rose' ? 'text-[var(--rose-soft)]' : 'text-[var(--sage-soft)]'
	);
</script>

<button
	type="button"
	class={`${baseClass} ${toneClass}`}
	aria-pressed={pressed}
	aria-label={ariaLabel}
	{disabled}
	onclick={handleClick}
	onpointerdown={startHold}
	onpointerup={stopHold}
	onpointerleave={stopHold}
	onpointercancel={stopHold}
>
	<span class="inline-flex items-center justify-center" aria-hidden="true">
		{#if kind === 'door'}
			<svg class="h-10 w-[34px]" viewBox="0 0 34 40" fill="none" xmlns="http://www.w3.org/2000/svg">
				<rect x="7" y="2" width="20" height="36" rx="3" stroke="currentColor" stroke-width="2" />
				<circle cx="21" cy="20" r="2" fill="currentColor" />
			</svg>
		{:else}
			<svg class="h-10 w-[34px]" viewBox="0 0 34 40" fill="none" xmlns="http://www.w3.org/2000/svg">
				<circle cx="17" cy="12" r="10" stroke="currentColor" stroke-width="2" />
				<rect x="16" y="22" width="2" height="8" fill="currentColor" />
				<rect x="13" y="30" width="8" height="6" rx="2" fill="currentColor" />
			</svg>
		{/if}
	</span>

	<span
		class="text-[36px] leading-none font-[var(--font-ui)] font-bold tracking-[-0.04em] max-[959px]:text-[30px]"
	>
		{label}
	</span>
	<span
		class={`text-[16px] leading-none font-[var(--font-ui)] font-bold max-[959px]:text-[18px] ${captionClass}`}
	>
		{caption}
	</span>
</button>
