<template>
  <div class="demo-container">
    <div class="main-layout">
      <!-- Zone graphique principale -->
      <svg viewBox="0 0 700 300" class="architecture-svg">
        <defs>
          <linearGradient id="data-gradient" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stop-color="#2196F3" />
            <stop offset="100%" stop-color="#9C27B0" />
          </linearGradient>
          <filter id="glow">
            <feGaussianBlur stdDeviation="2.5" result="coloredBlur"/>
            <feMerge>
              <feMergeNode in="coloredBlur"/>
              <feMergeNode in="SourceGraphic"/>
            </feMerge>
          </filter>
        </defs>

        <!-- COUCHE PERCEPTION -->
        <g class="layer-perception">
          <rect x="15" y="35" width="160" height="220" rx="10"
                fill="rgba(33, 150, 243, 0.08)" stroke="#2196F3" stroke-width="2" stroke-dasharray="6,3" />
          <text x="95" y="25" text-anchor="middle" class="layer-title" fill="#2196F3">PERCEPTION</text>

          <g v-for="(sensor, idx) in sensors" :key="sensor.id"
             :transform="`translate(95, ${85 + idx * 60})`"
             @click="toggleSensor(sensor)"
             class="sensor-group">
            <circle r="26"
                    :fill="sensor.active ? '#4CAF50' : '#e0e0e0'"
                    :stroke="sensor.active ? '#2E7D32' : '#999'"
                    stroke-width="2"
                    :class="{ 'sensor-active': sensor.active }" />
            <text y="7" text-anchor="middle" class="sensor-emoji">{{ sensor.icon }}</text>
            <text y="45" text-anchor="middle" class="sensor-name">{{ sensor.name }}</text>
          </g>
        </g>

        <!-- COUCHE RÉSEAU -->
        <g class="layer-network">
          <rect x="210" y="35" width="180" height="220" rx="10"
                fill="rgba(255, 152, 0, 0.08)" stroke="#FF9800" stroke-width="2" stroke-dasharray="6,3" />
          <text x="300" y="25" text-anchor="middle" class="layer-title" fill="#FF9800">RÉSEAU</text>

          <g transform="translate(300, 115)" @click="gatewayActive = !gatewayActive" class="gateway-group">
            <rect x="-50" y="-32" width="100" height="64" rx="8"
                  :fill="gatewayActive ? '#FF9800' : '#e0e0e0'"
                  :stroke="gatewayActive ? '#E65100' : '#999'"
                  stroke-width="2" />
            <text y="6" text-anchor="middle" class="gateway-emoji">📡</text>
            <text y="50" text-anchor="middle" class="gateway-name">Gateway</text>
          </g>

          <g transform="translate(300, 210)">
            <g v-for="(proto, idx) in protocols" :key="proto.id"
               :transform="`translate(${(idx - 1) * 55}, 0)`"
               @click="selectedProtocol = proto.id"
               class="protocol-group">
              <rect x="-22" y="-14" width="44" height="28" rx="5"
                    :fill="selectedProtocol === proto.id ? '#2196F3' : '#e8e8e8'"
                    stroke="#999" stroke-width="1" />
              <text y="5" text-anchor="middle"
                    :fill="selectedProtocol === proto.id ? '#fff' : '#333'"
                    class="protocol-name">{{ proto.name }}</text>
            </g>
          </g>
        </g>

        <!-- COUCHE APPLICATION -->
        <g class="layer-application">
          <rect x="425" y="35" width="260" height="220" rx="10"
                fill="rgba(156, 39, 176, 0.08)" stroke="#9C27B0" stroke-width="2" stroke-dasharray="6,3" />
          <text x="555" y="25" text-anchor="middle" class="layer-title" fill="#9C27B0">APPLICATION</text>

          <g transform="translate(555, 100)" @click="cloudActive = !cloudActive" class="cloud-group">
            <ellipse rx="55" ry="32"
                     :fill="cloudActive ? '#9C27B0' : '#e0e0e0'"
                     :stroke="cloudActive ? '#6A1B9A' : '#999'"
                     stroke-width="2" />
            <text y="8" text-anchor="middle" class="cloud-emoji">☁️</text>
            <text y="52" text-anchor="middle" class="cloud-name">Cloud</text>
          </g>

          <g transform="translate(500, 200)" class="app-group">
            <rect x="-28" y="-20" width="56" height="40" rx="6" fill="#e8f5e9" stroke="#4CAF50" stroke-width="2" />
            <text y="6" text-anchor="middle" class="app-emoji">📱</text>
            <text y="35" text-anchor="middle" class="app-name">Apps</text>
          </g>

          <g transform="translate(610, 200)" class="app-group">
            <rect x="-28" y="-20" width="56" height="40" rx="6" fill="#e3f2fd" stroke="#2196F3" stroke-width="2" />
            <text y="6" text-anchor="middle" class="app-emoji">📊</text>
            <text y="35" text-anchor="middle" class="app-name">Dashboard</text>
          </g>
        </g>

        <!-- CONNEXIONS -->
        <g class="connections">
          <path d="M 175 145 Q 240 130 250 115" stroke="#ccc" stroke-width="2" fill="none" stroke-dasharray="5,3" />
          <path d="M 350 115 Q 420 100 500 100" stroke="#ccc" stroke-width="2" fill="none" stroke-dasharray="5,3" />
        </g>

        <!-- PAQUETS DE DONNÉES -->
        <g class="data-packets">
          <g v-for="packet in dataPackets" :key="packet.id">
            <circle :cx="packet.x" :cy="packet.y" r="14"
                    fill="url(#data-gradient)"
                    filter="url(#glow)"
                    class="data-packet" />
            <text :x="packet.x" :y="packet.y + 5" text-anchor="middle" class="packet-emoji">
              {{ packet.icon }}
            </text>
          </g>
        </g>
      </svg>

      <!-- PANNEAU DE CONTRÔLE -->
      <div class="control-panel">
        <div class="control-section">
          <div class="section-title">Simulation</div>
          <button @click="toggleSimulation" :class="{ running: isSimulating }">
            {{ isSimulating ? '⏸ Pause' : '▶ Démarrer' }}
          </button>
          <button @click="resetAll" class="btn-reset">↺ Reset</button>
        </div>

        <div class="control-section">
          <div class="section-title">Stats</div>
          <div class="stat-item">
            <span>Capteurs</span>
            <span class="stat-value">{{ activeSensors.length }}/{{ sensors.length }}</span>
          </div>
          <div class="stat-item">
            <span>Paquets</span>
            <span class="stat-value">{{ packetCount }}</span>
          </div>
        </div>

        <div class="control-section">
          <div class="section-title">Activité</div>
          <div class="activity-log">
            <div v-for="msg in recentMessages" :key="msg.id" class="log-item">
              {{ msg.icon }} {{ msg.text }}
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'

const sensors = ref([
  { id: 1, name: 'Température', icon: '🌡️', active: true },
  { id: 2, name: 'Humidité', icon: '💧', active: true },
  { id: 3, name: 'Mouvement', icon: '👁️', active: false }
])

const protocols = [
  { id: 'mqtt', name: 'MQTT' },
  { id: 'wifi', name: 'WiFi' },
  { id: 'lora', name: 'LoRa' }
]

const gatewayActive = ref(true)
const cloudActive = ref(true)
const selectedProtocol = ref('mqtt')
const isSimulating = ref(false)
const packetCount = ref(0)
const dataPackets = ref([])
const recentMessages = ref([])

let animationFrame = null
let lastPacketTime = 0
let packetId = 0
let messageId = 0

const activeSensors = computed(() => sensors.value.filter(s => s.active))

function toggleSensor(sensor) {
  sensor.active = !sensor.active
  addMessage(sensor.icon, sensor.active ? 'ON' : 'OFF')
}

function addMessage(icon, text) {
  recentMessages.value.unshift({ id: messageId++, icon, text })
  if (recentMessages.value.length > 4) recentMessages.value.pop()
}

function createPacket() {
  if (activeSensors.value.length === 0 || !gatewayActive.value) return

  const sensor = activeSensors.value[Math.floor(Math.random() * activeSensors.value.length)]
  const sensorIdx = sensors.value.findIndex(s => s.id === sensor.id)
  const startY = 85 + sensorIdx * 60

  dataPackets.value.push({
    id: packetId++,
    x: 121,
    y: startY,
    startY: startY,
    progress: 0,
    phase: 1,
    icon: sensor.icon
  })

  packetCount.value++
  addMessage(sensor.icon, '→ Gateway')
}

function updatePackets(timestamp) {
  if (timestamp - lastPacketTime > 700) {
    createPacket()
    lastPacketTime = timestamp
  }

  dataPackets.value = dataPackets.value.filter(p => {
    p.progress += 0.025

    if (p.phase === 1) {
      p.x = 121 + (300 - 121) * p.progress
      p.y = p.startY + (115 - p.startY) * p.progress

      if (p.progress >= 1) {
        p.progress = 0
        p.phase = 2
        addMessage('📡', 'Reçu')
      }
    } else if (p.phase === 2 && cloudActive.value) {
      p.x = 300 + (555 - 300) * p.progress
      p.y = 115 + (100 - 115) * p.progress

      if (p.progress >= 1) {
        addMessage('☁️', 'Stocké')
        return false
      }
    } else if (!cloudActive.value) {
      return false
    }

    return true
  })

  if (isSimulating.value) {
    animationFrame = requestAnimationFrame(updatePackets)
  }
}

function toggleSimulation() {
  isSimulating.value = !isSimulating.value

  if (isSimulating.value) {
    lastPacketTime = 0
    animationFrame = requestAnimationFrame(updatePackets)
  } else {
    cancelAnimationFrame(animationFrame)
  }
}

function resetAll() {
  isSimulating.value = false
  cancelAnimationFrame(animationFrame)

  sensors.value[0].active = true
  sensors.value[1].active = true
  sensors.value[2].active = false
  gatewayActive.value = true
  cloudActive.value = true
  selectedProtocol.value = 'mqtt'
  packetCount.value = 0
  dataPackets.value = []
  recentMessages.value = []

  addMessage('🔄', 'Reset')
}

onMounted(() => {
  addMessage('✅', 'Prêt')
})

onUnmounted(() => {
  cancelAnimationFrame(animationFrame)
})
</script>

<style scoped>
.demo-container {
  padding: 8px;
  background: linear-gradient(135deg, #fafafa 0%, #f0f0f0 100%);
  border-radius: 10px;
}

.main-layout {
  display: flex;
  gap: 12px;
}

.architecture-svg {
  flex: 1;
  height: 300px;
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 6px rgba(0,0,0,0.1);
}

.layer-title {
  font-size: 14px;
  font-weight: bold;
  letter-spacing: 0.5px;
}

.sensor-group, .gateway-group, .cloud-group, .protocol-group {
  cursor: pointer;
  transition: transform 0.2s;
}
.sensor-group:hover, .gateway-group:hover, .cloud-group:hover {
  transform: scale(1.06);
}
.protocol-group:hover {
  transform: scale(1.08);
}

.sensor-emoji { font-size: 22px; }
.sensor-name { font-size: 11px; fill: #333; font-weight: 500; }
.sensor-active { filter: drop-shadow(0 0 6px rgba(76, 175, 80, 0.5)); }

.gateway-emoji { font-size: 26px; }
.gateway-name { font-size: 12px; fill: #333; font-weight: 500; }

.protocol-name { font-size: 10px; font-weight: bold; }

.cloud-emoji { font-size: 26px; }
.cloud-name { font-size: 12px; fill: #333; font-weight: 500; }

.app-emoji { font-size: 18px; }
.app-name { font-size: 10px; fill: #333; }

.data-packet { animation: pulse 0.35s ease-in-out infinite alternate; }
.packet-emoji { font-size: 11px; }

@keyframes pulse {
  from { opacity: 0.7; }
  to { opacity: 1; }
}

.control-panel {
  width: 140px;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.control-section {
  background: white;
  padding: 10px;
  border-radius: 8px;
  box-shadow: 0 1px 4px rgba(0,0,0,0.08);
}

.section-title {
  font-size: 11px;
  font-weight: bold;
  color: #333;
  margin-bottom: 8px;
  padding-bottom: 4px;
  border-bottom: 2px solid #eee;
}

button {
  width: 100%;
  padding: 8px;
  font-size: 11px;
  font-weight: bold;
  border: none;
  border-radius: 6px;
  cursor: pointer;
  margin-bottom: 6px;
  transition: all 0.2s;
  background: #e3f2fd;
  color: #1976D2;
}

button:hover {
  transform: translateY(-1px);
  box-shadow: 0 2px 6px rgba(0,0,0,0.12);
}

button.running {
  background: #4CAF50;
  color: white;
}

button.btn-reset {
  background: #ffebee;
  color: #c62828;
  margin-bottom: 0;
}

.stat-item {
  display: flex;
  justify-content: space-between;
  padding: 4px 0;
  font-size: 11px;
  border-bottom: 1px solid #f5f5f5;
}

.stat-value {
  font-weight: bold;
  color: #2196F3;
}

.activity-log {
  max-height: 70px;
  overflow-y: auto;
}

.log-item {
  font-size: 10px;
  padding: 3px 0;
  color: #555;
  border-bottom: 1px solid #f9f9f9;
}
</style>
