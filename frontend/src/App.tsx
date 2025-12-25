import { Component, createSignal, createEffect, onCleanup, For, Show } from 'solid-js';

// ============================================================================
// TypeScript Interfaces
// ============================================================================

interface SuccessResponse {
  status: string;
}

interface ErrorResponse {
  error: string;
}

interface SetAngleRequest {
  angle: number;
}

interface SetPwmRequest {
  pulse_us: number;
}

interface PoseRequest {
  angles: number[];
}

interface MoveRequest {
  duration_ms: number;
  angles: number[];
}

interface ServoPosition {
  channel: number;
  angle: number;
}

interface ServoPositions {
  servos: ServoPosition[];
}

type TabType = 'motors' | 'calibration' | 'pose' | 'move';

interface ConsoleMessage {
  type: 'success' | 'error' | 'info';
  text: string;
  timestamp: Date;
}

// ============================================================================
// API Client Functions
// ============================================================================

async function apiCall<T>(url: string, options?: RequestInit): Promise<T> {
  try {
    const response = await fetch(url, {
      headers: {
        'Content-Type': 'application/json',
        ...options?.headers,
      },
      ...options,
    });

    if (!response.ok) {
      const errorData: ErrorResponse = await response.json().catch(() => ({
        error: `HTTP ${response.status}: ${response.statusText}`,
      }));
      throw new Error(errorData.error || `HTTP ${response.status}`);
    }

    return await response.json();
  } catch (error) {
    if (error instanceof Error) {
      throw error;
    }
    throw new Error('Unknown error occurred');
  }
}

async function startSerialMode(): Promise<void> {
  await apiCall<SuccessResponse>('/api/serial/start', { method: 'POST' });
}

async function stopSerialMode(): Promise<void> {
  await apiCall<SuccessResponse>('/api/serial/stop', { method: 'POST' });
}

async function setServoAngle(id: number, angle: number): Promise<void> {
  const body: SetAngleRequest = { angle };
  await apiCall<SuccessResponse>(`/api/servo/${id}/angle`, {
    method: 'POST',
    body: JSON.stringify(body),
  });
}

async function setServoPWM(id: number, pulse_us: number): Promise<void> {
  const body: SetPwmRequest = { pulse_us };
  await apiCall<SuccessResponse>(`/api/servo/${id}/pwm`, {
    method: 'POST',
    body: JSON.stringify(body),
  });
}

async function executePose(angles: number[]): Promise<void> {
  const body: PoseRequest = { angles };
  await apiCall<SuccessResponse>('/api/pose', {
    method: 'POST',
    body: JSON.stringify(body),
  });
}

async function executeMove(duration_ms: number, angles: number[]): Promise<void> {
  const body: MoveRequest = { duration_ms, angles };
  await apiCall<SuccessResponse>('/api/move', {
    method: 'POST',
    body: JSON.stringify(body),
  });
}

// ============================================================================
// Main App Component
// ============================================================================

const App: Component = () => {
  // State Management
  const [numServos, setNumServos] = createSignal(6);
  const [serialMode, setSerialMode] = createSignal(false);
  const [currentTab, setCurrentTab] = createSignal<TabType>('motors');
  const [servoAngles, setServoAngles] = createSignal<number[]>(Array(6).fill(90));
  const [servoPWMs, setServoPWMs] = createSignal<number[]>(Array(6).fill(1500));
  const [poseAngles, setPoseAngles] = createSignal<number[]>(Array(6).fill(90));
  const [moveAngles, setMoveAngles] = createSignal<number[]>(Array(6).fill(90));
  const [moveDuration, setMoveDuration] = createSignal(1000);
  const [loading, setLoading] = createSignal(false);
  const [loadingServo, setLoadingServo] = createSignal<number | null>(null);
  const [consoleMessages, setConsoleMessages] = createSignal<ConsoleMessage[]>([]);

  // Effect: Reinitialize arrays when numServos changes
  createEffect(() => {
    const count = numServos();
    setServoAngles(Array(count).fill(90));
    setServoPWMs(Array(count).fill(1500));
    setPoseAngles(Array(count).fill(90));
    setMoveAngles(Array(count).fill(90));
  });

  // Cleanup: Stop serial mode on unmount
  onCleanup(async () => {
    if (serialMode()) {
      try {
        await stopSerialMode();
      } catch (error) {
        console.error('Failed to stop serial mode on cleanup:', error);
      }
    }
  });

  // Console management
  const addConsoleMessage = (type: 'success' | 'error' | 'info', text: string) => {
    const newMessage: ConsoleMessage = {
      type,
      text,
      timestamp: new Date(),
    };
    setConsoleMessages([...consoleMessages(), newMessage]);
  };

  const clearConsole = () => {
    setConsoleMessages([]);
  };

  // Serial mode toggle
  const toggleSerialMode = async () => {
    try {
      setLoading(true);
      if (serialMode()) {
        await stopSerialMode();
        setSerialMode(false);
        addConsoleMessage('success', 'Switched to Button Mode');
      } else {
        await startSerialMode();
        setSerialMode(true);
        addConsoleMessage('success', 'Switched to Serial Mode');
      }
    } catch (error) {
      addConsoleMessage('error', error instanceof Error ? error.message : 'Failed to toggle serial mode');
    } finally {
      setLoading(false);
    }
  };

  // Motors tab: Set servo angle
  const handleSetAngle = async (servoId: number, angle: number) => {
    try {
      setLoadingServo(servoId);
      await setServoAngle(servoId, angle);
      addConsoleMessage('success', `Servo ${servoId} set to ${angle}°`);
    } catch (error) {
      addConsoleMessage('error', error instanceof Error ? error.message : `Failed to set servo ${servoId}`);
    } finally {
      setLoadingServo(null);
    }
  };

  // Calibration tab: Set servo PWM
  const handleSetPWM = async (servoId: number, pwm: number) => {
    try {
      setLoadingServo(servoId);
      await setServoPWM(servoId, pwm);
      addConsoleMessage('success', `Servo ${servoId} PWM set to ${pwm}μs`);
    } catch (error) {
      addConsoleMessage('error', error instanceof Error ? error.message : `Failed to set PWM for servo ${servoId}`);
    } finally {
      setLoadingServo(null);
    }
  };

  // POSE tab: Execute pose
  const handleExecutePose = async () => {
    try {
      setLoading(true);
      await executePose(poseAngles());
      addConsoleMessage('success', 'POSE executed successfully');
    } catch (error) {
      addConsoleMessage('error', error instanceof Error ? error.message : 'Failed to execute POSE');
    } finally {
      setLoading(false);
    }
  };

  // MOVE tab: Execute move
  const handleExecuteMove = async () => {
    try {
      setLoading(true);
      await executeMove(moveDuration(), moveAngles());
      addConsoleMessage('success', `MOVE executed (${moveDuration()}ms)`);
    } catch (error) {
      addConsoleMessage('error', error instanceof Error ? error.message : 'Failed to execute MOVE');
    } finally {
      setLoading(false);
    }
  };

  // ============================================================================
  // Render
  // ============================================================================

  return (
    <div class="min-h-screen bg-gray-100 pb-40">
      {/* Header */}
      <header class="bg-white shadow">
        <div class="max-w-7xl mx-auto px-4 py-6 sm:px-6 lg:px-8">
          <div class="flex flex-col sm:flex-row justify-between items-start sm:items-center gap-4">
            <h1 class="text-3xl font-bold text-gray-900">Robot Arm Controller</h1>

            <div class="flex flex-col sm:flex-row items-start sm:items-center gap-4">
              {/* Number of Servos Input */}
              <div class="flex items-center gap-2">
                <label for="num-servos" class="text-sm font-medium text-gray-700">
                  Servos:
                </label>
                <input
                  id="num-servos"
                  type="number"
                  min="1"
                  max="16"
                  value={numServos()}
                  onInput={(e) => {
                    const val = parseInt(e.currentTarget.value);
                    if (val >= 1 && val <= 16) {
                      setNumServos(val);
                    }
                  }}
                  class="w-16 px-2 py-1 border border-gray-300 rounded-md text-sm"
                />
              </div>

              {/* Serial Mode Toggle */}
              <button
                onClick={toggleSerialMode}
                disabled={loading()}
                class={`px-4 py-2 rounded-md font-medium text-white transition-colors ${
                  serialMode()
                    ? 'bg-green-600 hover:bg-green-700'
                    : 'bg-gray-600 hover:bg-gray-700'
                } disabled:opacity-50 disabled:cursor-not-allowed`}
              >
                {serialMode() ? 'Serial Mode' : 'Button Mode'}
              </button>
            </div>
          </div>
        </div>
      </header>

      {/* Tab Navigation */}
      <div class="max-w-7xl mx-auto px-4 py-6 sm:px-6 lg:px-8">
        <div class="bg-white rounded-lg shadow">
          {/* Tab Headers */}
          <div class="border-b border-gray-200">
            <nav class="flex -mb-px">
              <button
                onClick={() => setCurrentTab('motors')}
                class={`px-6 py-3 text-sm font-medium border-b-2 transition-colors ${
                  currentTab() === 'motors'
                    ? 'border-blue-500 text-blue-600'
                    : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
                }`}
              >
                Motors
              </button>
              <button
                onClick={() => setCurrentTab('calibration')}
                class={`px-6 py-3 text-sm font-medium border-b-2 transition-colors ${
                  currentTab() === 'calibration'
                    ? 'border-blue-500 text-blue-600'
                    : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
                }`}
              >
                Calibration
              </button>
              <button
                onClick={() => setCurrentTab('pose')}
                class={`px-6 py-3 text-sm font-medium border-b-2 transition-colors ${
                  currentTab() === 'pose'
                    ? 'border-blue-500 text-blue-600'
                    : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
                }`}
              >
                POSE
              </button>
              <button
                onClick={() => setCurrentTab('move')}
                class={`px-6 py-3 text-sm font-medium border-b-2 transition-colors ${
                  currentTab() === 'move'
                    ? 'border-blue-500 text-blue-600'
                    : 'border-transparent text-gray-500 hover:text-gray-700 hover:border-gray-300'
                }`}
              >
                MOVE
              </button>
            </nav>
          </div>

          {/* Tab Content */}
          <div class="p-6">
            {/* Motors Tab */}
            <Show when={currentTab() === 'motors'}>
              <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
                <For each={Array(numServos()).fill(0)}>
                  {(_, index) => {
                    const servoId = index();
                    return (
                      <div class="bg-gray-50 p-4 rounded-lg border border-gray-200">
                        <h3 class="text-lg font-semibold mb-3">Servo {servoId}</h3>

                        <div class="mb-3">
                          <label class="block text-sm font-medium text-gray-700 mb-1">
                            Angle: {servoAngles()[servoId]}°
                          </label>
                          <input
                            type="range"
                            min="0"
                            max="180"
                            step="5"
                            value={servoAngles()[servoId]}
                            onInput={(e) => {
                              const newAngles = [...servoAngles()];
                              newAngles[servoId] = parseInt(e.currentTarget.value);
                              setServoAngles(newAngles);
                            }}
                            disabled={!serialMode()}
                            class="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer disabled:opacity-50"
                          />
                        </div>

                        <div class="flex items-center gap-2 mb-3">
                          <button
                            onClick={() => {
                              const newAngles = [...servoAngles()];
                              newAngles[servoId] = Math.max(0, newAngles[servoId] - 5);
                              setServoAngles(newAngles);
                            }}
                            disabled={!serialMode()}
                            class="px-3 py-1 bg-gray-200 rounded hover:bg-gray-300 disabled:opacity-50"
                          >
                            -
                          </button>
                          <input
                            type="number"
                            min="0"
                            max="180"
                            value={servoAngles()[servoId]}
                            onInput={(e) => {
                              const val = parseInt(e.currentTarget.value);
                              if (val >= 0 && val <= 180) {
                                const newAngles = [...servoAngles()];
                                newAngles[servoId] = val;
                                setServoAngles(newAngles);
                              }
                            }}
                            disabled={!serialMode()}
                            class="flex-1 px-2 py-1 border border-gray-300 rounded text-center disabled:opacity-50"
                          />
                          <button
                            onClick={() => {
                              const newAngles = [...servoAngles()];
                              newAngles[servoId] = Math.min(180, newAngles[servoId] + 5);
                              setServoAngles(newAngles);
                            }}
                            disabled={!serialMode()}
                            class="px-3 py-1 bg-gray-200 rounded hover:bg-gray-300 disabled:opacity-50"
                          >
                            +
                          </button>
                        </div>

                        <button
                          onClick={() => handleSetAngle(servoId, servoAngles()[servoId])}
                          disabled={!serialMode() || loadingServo() === servoId}
                          class="w-full px-4 py-2 bg-blue-600 text-white rounded-md hover:bg-blue-700 disabled:opacity-50 disabled:cursor-not-allowed"
                        >
                          {loadingServo() === servoId ? 'Setting...' : 'Set Angle'}
                        </button>
                      </div>
                    );
                  }}
                </For>
              </div>
            </Show>

            {/* Calibration Tab */}
            <Show when={currentTab() === 'calibration'}>
              <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
                <For each={Array(numServos()).fill(0)}>
                  {(_, index) => {
                    const servoId = index();
                    return (
                      <div class="bg-gray-50 p-4 rounded-lg border border-gray-200">
                        <h3 class="text-lg font-semibold mb-3">Servo {servoId}</h3>

                        <div class="mb-3">
                          <label class="block text-sm font-medium text-gray-700 mb-1">
                            PWM: {servoPWMs()[servoId]}μs
                          </label>
                          <input
                            type="range"
                            min="0"
                            max="20000"
                            step="10"
                            value={servoPWMs()[servoId]}
                            onInput={(e) => {
                              const newPWMs = [...servoPWMs()];
                              newPWMs[servoId] = parseInt(e.currentTarget.value);
                              setServoPWMs(newPWMs);
                            }}
                            disabled={!serialMode()}
                            class="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer disabled:opacity-50"
                          />
                        </div>

                        <div class="mb-3">
                          <input
                            type="number"
                            min="0"
                            max="20000"
                            step="10"
                            value={servoPWMs()[servoId]}
                            onInput={(e) => {
                              const val = parseInt(e.currentTarget.value);
                              if (val >= 0 && val <= 20000) {
                                const newPWMs = [...servoPWMs()];
                                newPWMs[servoId] = val;
                                setServoPWMs(newPWMs);
                              }
                            }}
                            disabled={!serialMode()}
                            class="w-full px-3 py-2 border border-gray-300 rounded disabled:opacity-50"
                            placeholder="0-20000"
                          />
                        </div>

                        <button
                          onClick={() => handleSetPWM(servoId, servoPWMs()[servoId])}
                          disabled={!serialMode() || loadingServo() === servoId}
                          class="w-full px-4 py-2 bg-blue-600 text-white rounded-md hover:bg-blue-700 disabled:opacity-50 disabled:cursor-not-allowed"
                        >
                          {loadingServo() === servoId ? 'Setting...' : 'Set PWM'}
                        </button>
                      </div>
                    );
                  }}
                </For>
              </div>
            </Show>

            {/* POSE Tab */}
            <Show when={currentTab() === 'pose'}>
              <div class="max-w-4xl mx-auto">
                <div class="mb-6">
                  <h2 class="text-xl font-semibold mb-4">Set All Servo Angles (Instant)</h2>
                  <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
                    <For each={Array(numServos()).fill(0)}>
                      {(_, index) => {
                        const servoId = index();
                        return (
                          <div class="bg-gray-50 p-4 rounded-lg border border-gray-200">
                            <label class="block text-sm font-medium text-gray-700 mb-2">
                              Servo {servoId}: {poseAngles()[servoId]}°
                            </label>
                            <div class="flex items-center gap-2">
                              <input
                                type="range"
                                min="0"
                                max="180"
                                step="5"
                                value={poseAngles()[servoId]}
                                onInput={(e) => {
                                  const newAngles = [...poseAngles()];
                                  newAngles[servoId] = parseInt(e.currentTarget.value);
                                  setPoseAngles(newAngles);
                                }}
                                disabled={!serialMode()}
                                class="flex-1 h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer disabled:opacity-50"
                              />
                              <input
                                type="number"
                                min="0"
                                max="180"
                                value={poseAngles()[servoId]}
                                onInput={(e) => {
                                  const val = parseInt(e.currentTarget.value);
                                  if (val >= 0 && val <= 180) {
                                    const newAngles = [...poseAngles()];
                                    newAngles[servoId] = val;
                                    setPoseAngles(newAngles);
                                  }
                                }}
                                disabled={!serialMode()}
                                class="w-16 px-2 py-1 border border-gray-300 rounded text-center text-sm disabled:opacity-50"
                              />
                            </div>
                          </div>
                        );
                      }}
                    </For>
                  </div>
                </div>

                <button
                  onClick={handleExecutePose}
                  disabled={!serialMode() || loading()}
                  class="w-full px-6 py-4 bg-green-600 text-white text-lg font-semibold rounded-md hover:bg-green-700 disabled:opacity-50 disabled:cursor-not-allowed"
                >
                  {loading() ? 'Executing...' : 'Execute POSE'}
                </button>
              </div>
            </Show>

            {/* MOVE Tab */}
            <Show when={currentTab() === 'move'}>
              <div class="max-w-4xl mx-auto">
                {/* Duration Section */}
                <div class="mb-6 bg-gray-50 p-4 rounded-lg border border-gray-200">
                  <label class="block text-sm font-medium text-gray-700 mb-2">
                    Duration: {moveDuration()}ms
                  </label>
                  <input
                    type="range"
                    min="100"
                    max="10000"
                    step="100"
                    value={moveDuration()}
                    onInput={(e) => setMoveDuration(parseInt(e.currentTarget.value))}
                    disabled={!serialMode()}
                    class="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer disabled:opacity-50"
                  />
                </div>

                {/* Servo Angles Section */}
                <div class="mb-6">
                  <h2 class="text-xl font-semibold mb-4">Set Target Angles (Interpolated)</h2>
                  <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
                    <For each={Array(numServos()).fill(0)}>
                      {(_, index) => {
                        const servoId = index();
                        return (
                          <div class="bg-gray-50 p-4 rounded-lg border border-gray-200">
                            <label class="block text-sm font-medium text-gray-700 mb-2">
                              Servo {servoId}: {moveAngles()[servoId]}°
                            </label>
                            <div class="flex items-center gap-2">
                              <input
                                type="range"
                                min="0"
                                max="180"
                                step="5"
                                value={moveAngles()[servoId]}
                                onInput={(e) => {
                                  const newAngles = [...moveAngles()];
                                  newAngles[servoId] = parseInt(e.currentTarget.value);
                                  setMoveAngles(newAngles);
                                }}
                                disabled={!serialMode()}
                                class="flex-1 h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer disabled:opacity-50"
                              />
                              <input
                                type="number"
                                min="0"
                                max="180"
                                value={moveAngles()[servoId]}
                                onInput={(e) => {
                                  const val = parseInt(e.currentTarget.value);
                                  if (val >= 0 && val <= 180) {
                                    const newAngles = [...moveAngles()];
                                    newAngles[servoId] = val;
                                    setMoveAngles(newAngles);
                                  }
                                }}
                                disabled={!serialMode()}
                                class="w-16 px-2 py-1 border border-gray-300 rounded text-center text-sm disabled:opacity-50"
                              />
                            </div>
                          </div>
                        );
                      }}
                    </For>
                  </div>
                </div>

                <button
                  onClick={handleExecuteMove}
                  disabled={!serialMode() || loading()}
                  class="w-full px-6 py-4 bg-green-600 text-white text-lg font-semibold rounded-md hover:bg-green-700 disabled:opacity-50 disabled:cursor-not-allowed"
                >
                  {loading() ? 'Executing...' : 'Execute MOVE'}
                </button>
              </div>
            </Show>
          </div>
        </div>
      </div>

      {/* Console Log */}
      <div class="fixed bottom-0 left-0 right-0 bg-gray-900 text-gray-100 border-t border-gray-700 z-50">
        <div class="max-w-7xl mx-auto px-4 py-2">
          <div class="flex justify-between items-center mb-2">
            <h3 class="text-sm font-semibold text-gray-400">Console</h3>
            <button
              onClick={clearConsole}
              class="text-xs text-gray-400 hover:text-gray-200 px-2 py-1 rounded hover:bg-gray-800"
            >
              Clear
            </button>
          </div>
          <div class="max-h-32 overflow-y-auto font-mono text-xs space-y-1">
            <Show when={consoleMessages().length === 0}>
              <div class="text-gray-500 italic">No messages</div>
            </Show>
            <For each={consoleMessages()}>
              {(msg) => (
                <div class="flex items-start gap-2">
                  <span class="text-gray-500 shrink-0">
                    {msg.timestamp.toLocaleTimeString()}
                  </span>
                  <span
                    class={`shrink-0 ${
                      msg.type === 'success'
                        ? 'text-green-400'
                        : msg.type === 'error'
                        ? 'text-red-400'
                        : 'text-blue-400'
                    }`}
                  >
                    [{msg.type.toUpperCase()}]
                  </span>
                  <span class="text-gray-200">{msg.text}</span>
                </div>
              )}
            </For>
          </div>
        </div>
      </div>
    </div>
  );
};

export default App;
