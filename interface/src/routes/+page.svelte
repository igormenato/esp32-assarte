<script lang="ts">
	import { onMount } from 'svelte';

	import AccentBar from '$lib/components/AccentBar.svelte';
	import ControlTile from '$lib/components/ControlTile.svelte';

	type ControlKind = 'door' | 'light';
	type ControlTone = 'rose' | 'sage';
	type DeviceState = {
		door: 'open' | 'closed';
		light: 'on' | 'off';
		doorSource?: 'last_known';
	};
	type PendingAction =
		| 'door-open'
		| 'door-close'
		| 'door-mark-open'
		| 'door-mark-closed'
		| 'light-on'
		| 'light-off'
		| 'light-mark-on'
		| 'light-mark-off'
		| null;

	type Control = {
		id: ControlKind;
		active: boolean;
		caption: string;
		label: string;
		tone: ControlTone;
		pending: boolean;
		toggle: () => Promise<void>;
		onHold?: () => Promise<void>;
	};

	const STATE_REQUEST_TIMEOUT_MS = 4000;
	const COMMAND_REQUEST_TIMEOUT_MS = 5000;

	let doorOpen = $state(false);
	let lightOn = $state(true);
	let doorSource = $state<DeviceState['doorSource'] | null>(null);
	let pendingAction = $state<PendingAction>(null);
	let stateStatus = $state<'loading' | 'ready' | 'error'>('loading');
	let stateSyncPending = $state(false);
	let statusText = $state('');

	const doorAccentTone = $derived<ControlTone>(doorOpen ? 'sage' : 'rose');
	const lightAccentTone = $derived<ControlTone>(lightOn ? 'sage' : 'rose');
	const isReady = $derived(stateStatus === 'ready');
	const visibleStatusText = $derived(
		stateStatus === 'loading' || stateSyncPending ? 'Sincronizando estado do ESP32...' : statusText
	);
	const sharedHintText = $derived('Segure o botão por 5s para corrigir o estado salvo');

	function applyDeviceState(state: DeviceState) {
		doorOpen = state.door === 'open';
		lightOn = state.light === 'on';
		doorSource = state.doorSource ?? null;
	}

	async function requestDeviceState(path: string, init: RequestInit, timeoutMs: number) {
		const controller = new AbortController();
		const timeoutId = window.setTimeout(() => controller.abort(), timeoutMs);

		try {
			const response = await fetch(path, {
				...init,
				signal: controller.signal
			});

			if (!response.ok) {
				throw new Error(`Request failed with ${response.status}`);
			}

			return (await response.json()) as DeviceState;
		} catch (error) {
			if (error instanceof DOMException && error.name === 'AbortError') {
				throw new Error(`Request timed out after ${timeoutMs}ms`);
			}

			throw error;
		} finally {
			window.clearTimeout(timeoutId);
		}
	}

	async function fetchDeviceState() {
		const state = await requestDeviceState(
			'/api/state',
			{
				headers: {
					accept: 'application/json'
				}
			},
			STATE_REQUEST_TIMEOUT_MS
		);
		applyDeviceState(state);
	}

	async function syncDeviceState() {
		if (stateSyncPending) {
			return;
		}

		stateSyncPending = true;

		try {
			await fetchDeviceState();
			stateStatus = 'ready';
			statusText = '';
		} catch (error) {
			stateStatus = 'error';
			throw error;
		} finally {
			stateSyncPending = false;
		}
	}

	async function retryStateSync() {
		statusText = '';
		stateStatus = 'loading';

		try {
			await syncDeviceState();
		} catch (error) {
			console.error(error);
			statusText = 'Nao foi possivel carregar o estado do ESP32.';
		}
	}

	async function sendDeviceCommand(action: Exclude<PendingAction, null>, endpoint: string) {
		if (pendingAction || !isReady || stateSyncPending) {
			return;
		}

		pendingAction = action;
		statusText = '';

		try {
			const state = await requestDeviceState(
				endpoint,
				{
					method: 'POST',
					headers: {
						accept: 'application/json'
					}
				},
				COMMAND_REQUEST_TIMEOUT_MS
			);

			applyDeviceState(state);
			stateStatus = 'ready';
		} catch (error) {
			console.error(error);

			try {
				await syncDeviceState();
				statusText = 'Comando nao confirmado. Estado atualizado pelo ESP32.';
			} catch (stateError) {
				console.error(stateError);
				statusText = 'Nao foi possivel confirmar o estado do ESP32.';
			}
		} finally {
			pendingAction = null;
		}
	}

	onMount(async () => {
		try {
			await syncDeviceState();
		} catch (error) {
			console.error(error);
			statusText = 'Nao foi possivel carregar o estado do ESP32.';
		}
	});

	const controls = $derived<Control[]>([
		{
			id: 'door',
			active: doorOpen,
			caption: 'Porta',
			label: isReady ? (doorOpen ? 'ABERTO' : 'FECHADO') : '---',
			tone: isReady ? (doorOpen ? 'sage' : 'rose') : 'rose',
			pending:
				pendingAction === 'door-open' ||
				pendingAction === 'door-close' ||
				pendingAction === 'door-mark-open' ||
				pendingAction === 'door-mark-closed',
			toggle: () =>
				sendDeviceCommand(
					doorOpen ? 'door-close' : 'door-open',
					doorOpen ? '/api/door/closed' : '/api/door/open'
				),
			onHold: () =>
				sendDeviceCommand(
					doorOpen ? 'door-mark-closed' : 'door-mark-open',
					doorOpen ? '/api/door/mark-closed' : '/api/door/mark-open'
				)
		},
		{
			id: 'light',
			active: lightOn,
			caption: 'Luz',
			label: isReady ? (lightOn ? 'LIGADO' : 'DESLIGADO') : '---',
			tone: isReady ? (lightOn ? 'sage' : 'rose') : 'sage',
			pending:
				pendingAction === 'light-on' ||
				pendingAction === 'light-off' ||
				pendingAction === 'light-mark-on' ||
				pendingAction === 'light-mark-off',
			toggle: () =>
				sendDeviceCommand(
					lightOn ? 'light-off' : 'light-on',
					lightOn ? '/api/light/off' : '/api/light/on'
				),
			onHold: () =>
				sendDeviceCommand(
					lightOn ? 'light-mark-off' : 'light-mark-on',
					lightOn ? '/api/light/mark-off' : '/api/light/mark-on'
				)
		}
	]);
</script>

<svelte:head>
	<title>Casa Inteligente</title>
	<meta name="theme-color" content="#ede2d2" />
</svelte:head>

<div
	class="flex min-h-svh items-center justify-center bg-(--bg) px-16 py-14 max-[959px]:px-6 max-[959px]:py-6"
>
	<main
		class="flex w-full max-w-360 flex-col items-center justify-center gap-20 max-[959px]:max-w-97.5 max-[959px]:gap-6"
		aria-label="Painel de casa inteligente"
	>
		<section class="flex flex-col items-center gap-6 max-[959px]:hidden">
			<h1
				class="m-0 text-center text-[72px] leading-none font-bold tracking-[-0.04em] text-(--ink)"
			>
				Casa Inteligente
			</h1>
			<AccentBar leftTone={doorAccentTone} rightTone={lightAccentTone} className="h-3 w-45" />
		</section>

		<section
			class="relative flex flex-col items-center gap-20 max-[959px]:w-full max-[959px]:gap-6"
		>
			<header
				class="hidden w-full items-center max-[959px]:flex max-[959px]:flex-col max-[959px]:gap-3"
			>
				<h1
					class="m-0 w-full text-center text-[34px] leading-none font-bold tracking-[-0.03em] text-(--ink)"
				>
					Casa Inteligente
				</h1>
				<AccentBar leftTone={doorAccentTone} rightTone={lightAccentTone} className="h-2 w-20" />
			</header>

			<div
				class="grid w-full max-w-250 grid-cols-[360px_360px] justify-center gap-25 rounded-[40px] bg-(--panel) p-20 max-[959px]:max-w-none max-[959px]:grid-cols-1 max-[959px]:gap-8 max-[959px]:rounded-[20px] max-[959px]:px-6 max-[959px]:pt-6 max-[959px]:pb-8 max-[959px]:shadow-(--panel-shadow)"
				role="group"
				aria-label="Controles da casa"
			>
				{#each controls as control (control.id)}
					<ControlTile
						kind={control.id}
						label={control.label}
						caption={control.caption}
						tone={control.tone}
						pressed={control.active}
						disabled={!isReady || stateSyncPending || control.pending}
						ariaLabel={`${control.caption} ${control.label.toLowerCase()}`}
						onToggle={control.toggle}
						onHold={control.onHold}
					/>
				{/each}

				<p
					class="col-span-2 m-0 text-center text-[16px] leading-none font-(--font-ui) text-[#A58D7C] max-[959px]:col-span-1 max-[959px]:text-[13px]"
				>
					{sharedHintText}
				</p>
			</div>

			{#if visibleStatusText}
				<p class="m-0 text-center text-sm font-(--font-ui) text-(--rose-strong)" role="status">
					{visibleStatusText}
				</p>
			{/if}

			{#if stateStatus === 'error'}
				<button
					type="button"
					class="absolute right-4 bottom-4 rounded-full border border-(--rose-soft) px-4 py-2 text-xs font-semibold tracking-[0.08em] text-(--rose-strong) uppercase transition-colors hover:bg-(--rose)/10 focus-visible:outline focus-visible:outline-offset-2 focus-visible:outline-(--rose-soft) max-[959px]:static max-[959px]:self-center"
					onclick={retryStateSync}
					disabled={stateSyncPending}
				>
					Tentar novamente
				</button>
			{/if}
		</section>
	</main>
</div>
