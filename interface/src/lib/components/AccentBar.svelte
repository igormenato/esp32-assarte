<script lang="ts">
	type ControlTone = 'rose' | 'sage';

	type Props = {
		leftTone: ControlTone;
		rightTone: ControlTone;
		className?: string;
	};

	let { leftTone, rightTone, className = '' }: Props = $props();

	const leftToneClass = $derived(leftTone === 'sage' ? 'bg-[var(--sage)]' : 'bg-[var(--rose)]');
	const rightToneClass = $derived(rightTone === 'sage' ? 'bg-[var(--sage)]' : 'bg-[var(--rose)]');
	const unified = $derived(leftTone === rightTone);
</script>

<div
	class={`flex items-stretch justify-center transition-[gap] duration-300 ease-out ${unified ? 'gap-0' : 'gap-1.5'} ${className}`}
	aria-hidden="true"
>
	<span
		class={`h-full flex-1 shadow-[inset_0_1px_0_rgba(255,255,255,0.3)] transition-[background-color,border-radius] duration-300 ease-out ${
			unified ? 'rounded-l-[4px] rounded-r-none' : 'rounded-[4px]'
		} ${leftToneClass}`}
	></span>
	<span
		class={`h-full flex-1 shadow-[inset_0_1px_0_rgba(255,255,255,0.3)] transition-[background-color,border-radius] duration-300 ease-out ${
			unified ? 'rounded-r-[4px] rounded-l-none' : 'rounded-[4px]'
		} ${rightToneClass}`}
	></span>
</div>
