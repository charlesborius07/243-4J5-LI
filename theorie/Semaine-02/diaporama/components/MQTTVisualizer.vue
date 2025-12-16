<template>
  <div class="demo-container">
    <div class="main-container">
      <!-- Zone SVG principale -->
      <div class="graph-area">
        <svg :viewBox="`0 0 ${svgWidth} ${svgHeight}`" class="mqtt-svg">
          <defs>
            <marker id="mqtt-arrow" markerWidth="6" markerHeight="4" refX="5" refY="2" orient="auto">
              <polygon points="0 0, 6 2, 0 4" fill="#2196F3" />
            </marker>
            <marker id="mqtt-arrow-green" markerWidth="6" markerHeight="4" refX="5" refY="2" orient="auto">
              <polygon points="0 0, 6 2, 0 4" fill="#4CAF50" />
            </marker>
          </defs>

          <!-- Zone Publishers -->
          <g class="publishers-zone">
            <rect x="5" y="30" width="100" height="200" rx="6"
                  fill="rgba(33, 150, 243, 0.1)" stroke="#2196F3" stroke-width="1" stroke-dasharray="4,2" />
            <text x="55" y="22" text-anchor="middle" class="zone-title" fill="#2196F3">Publishers</text>

            <g v-for="(pub, idx) in publishers" :key="pub.id"
               :transform="`translate(55, ${65 + idx * 55})`"
               class="publisher-node"
               @click="selectPublisher(pub)">
              <rect x="-40" y="-22" width="80" height="44" rx="6"
                    :fill="selectedPublisher?.id === pub.id ? '#2196F3' : '#e3f2fd'"
                    :stroke="pub.active ? '#2196F3' : '#90CAF9'" stroke-width="1.5" />
              <text y="-6" text-anchor="middle" class="node-icon">{{ pub.icon }}</text>
              <text y="8" text-anchor="middle" class="node-name"
                    :fill="selectedPublisher?.id === pub.id ? '#fff' : '#333'">{{ pub.name }}</text>
              <text y="18" text-anchor="middle" class="node-topic"
                    :fill="selectedPublisher?.id === pub.id ? '#e3f2fd' : '#666'">{{ pub.topic }}</text>
            </g>
          </g>

          <!-- Broker Central -->
          <g class="broker-zone" transform="translate(230, 130)">
            <rect x="-50" y="-60" width="100" height="120" rx="10"
                  :fill="brokerActive ? '#fff3e0' : '#f5f5f5'"
                  :stroke="brokerActive ? '#FF9800' : '#ccc'" stroke-width="2" />
            <text y="-40" text-anchor="middle" class="broker-title">Broker</text>
            <text y="-25" text-anchor="middle" class="broker-name">Mosquitto</text>

            <!-- File d'attente visuelle -->
            <g transform="translate(0, 10)">
              <rect x="-35" y="-20" width="70" height="40" rx="4" fill="#fff" stroke="#ddd" />
              <text y="-8" text-anchor="middle" class="queue-label">Messages</text>
              <g v-for="(msg, idx) in messageQueue.slice(0, 3)" :key="msg.id"
                 :transform="`translate(${-20 + idx * 20}, 8)`">
                <circle r="6" :fill="msg.color" />
                <text y="3" text-anchor="middle" class="queue-msg">{{ msg.count }}</text>
              </g>
            </g>

            <!-- Indicateur d'activité -->
            <circle cx="35" cy="-45" r="5"
                    :fill="brokerActive ? '#4CAF50' : '#ccc'" class="status-led" />
          </g>

          <!-- Zone Subscribers -->
          <g class="subscribers-zone">
            <rect x="345" y="30" width="105" height="200" rx="6"
                  fill="rgba(76, 175, 80, 0.1)" stroke="#4CAF50" stroke-width="1" stroke-dasharray="4,2" />
            <text x="398" y="22" text-anchor="middle" class="zone-title" fill="#4CAF50">Subscribers</text>

            <g v-for="(sub, idx) in subscribers" :key="sub.id"
               :transform="`translate(398, ${65 + idx * 55})`"
               class="subscriber-node"
               @click="toggleSubscription(sub)">
              <rect x="-42" y="-22" width="84" height="44" rx="6"
                    :fill="sub.subscribed ? '#e8f5e9' : '#f5f5f5'"
                    :stroke="sub.subscribed ? '#4CAF50' : '#ccc'" stroke-width="1.5" />
              <text y="-6" text-anchor="middle" class="node-icon">{{ sub.icon }}</text>
              <text y="8" text-anchor="middle" class="node-name">{{ sub.name }}</text>
              <text y="18" text-anchor="middle" class="node-topic"
                    :fill="sub.subscribed ? '#4CAF50' : '#999'">{{ sub.filter }}</text>
            </g>
          </g>

          <!-- Connexions et messages animés -->
          <g class="connections">
            <!-- Lignes de connexion Publishers -> Broker -->
            <g v-for="(pub, idx) in publishers" :key="'pub-line-'+pub.id">
              <line :x1="95" :y1="65 + idx * 55" x1="180" y1="130"
                    stroke="#e0e0e0" stroke-width="1" stroke-dasharray="3,2" />
            </g>

            <!-- Lignes de connexion Broker -> Subscribers -->
            <g v-for="(sub, idx) in subscribers" :key="'sub-line-'+sub.id">
              <line x1="280" y1="130" :x2="356" :y2="65 + idx * 55"
                    :stroke="sub.subscribed ? '#c8e6c9' : '#e0e0e0'"
                    stroke-width="1" stroke-dasharray="3,2" />
            </g>

            <!-- Messages en transit -->
            <g v-for="msg in messagesInTransit" :key="msg.id">
              <circle :cx="msg.x" :cy="msg.y" r="5"
                      :fill="msg.phase === 'publish' ? '#2196F3' : '#4CAF50'"
                      class="transit-message" />
              <text :x="msg.x" :y="msg.y + 3" text-anchor="middle" class="transit-icon">
                {{ msg.icon }}
              </text>
            </g>
          </g>

          <!-- Légende QoS -->
          <g transform="translate(115, 245)">
            <text x="0" y="0" class="legend-text">QoS:</text>
            <circle cx="25" cy="-3" r="4" fill="#4CAF50" />
            <text x="32" y="0" class="legend-text">0</text>
            <circle cx="50" cy="-3" r="4" fill="#FF9800" />
            <text x="57" y="0" class="legend-text">1</text>
            <circle cx="75" cy="-3" r="4" fill="#F44336" />
            <text x="82" y="0" class="legend-text">2</text>
          </g>
        </svg>
      </div>

      <!-- Panneau de contrôle -->
      <div class="controls">
        <div class="section">
          <h4>Publication</h4>
          <div class="publish-form">
            <label>Topic:</label>
            <input v-model="publishTopic" type="text" placeholder="maison/salon/temp" />
            <label>Message:</label>
            <input v-model="publishMessage" type="text" placeholder="25.5" />
            <label>QoS:</label>
            <select v-model="publishQoS">
              <option value="0">0 - At most once</option>
              <option value="1">1 - At least once</option>
              <option value="2">2 - Exactly once</option>
            </select>
            <button @click="publishMsg" :disabled="!canPublish" class="publish-btn">
              📤 Publier
            </button>
          </div>
        </div>

        <div class="section">
          <h4>Statistiques</h4>
          <div class="stat-row">
            <span>Messages publiés:</span>
            <span class="stat-value">{{ stats.published }}</span>
          </div>
          <div class="stat-row">
            <span>Messages livrés:</span>
            <span class="stat-value">{{ stats.delivered }}</span>
          </div>
          <div class="stat-row">
            <span>Subscribers actifs:</span>
            <span class="stat-value">{{ activeSubscribers.length }}</span>
          </div>
        </div>

        <div class="section">
          <h4>Journal</h4>
          <div class="log-area">
            <div v-for="log in logs" :key="log.id" class="log-entry" :class="log.type">
              <span class="log-icon">{{ log.icon }}</span>
              <span class="log-text">{{ log.text }}</span>
            </div>
          </div>
        </div>

        <div class="section">
          <button @click="resetDemo" class="reset-btn">↺ Réinitialiser</button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onUnmounted } from 'vue'

const svgWidth = 460
const svgHeight = 260

const publishers = ref([
  { id: 1, name: 'Capteur Temp', icon: '🌡️', topic: 'sensors/temp', active: true },
  { id: 2, name: 'Capteur Humid', icon: '💧', topic: 'sensors/humid', active: true },
  { id: 3, name: 'Détecteur Mouv', icon: '👁️', topic: 'sensors/motion', active: true }
])

const subscribers = ref([
  { id: 1, name: 'Dashboard', icon: '📊', filter: 'sensors/#', subscribed: true },
  { id: 2, name: 'App Mobile', icon: '📱', filter: 'sensors/temp', subscribed: true },
  { id: 3, name: 'Alertes', icon: '🔔', filter: 'sensors/motion', subscribed: false }
])

const selectedPublisher = ref(null)
const brokerActive = ref(true)
const messageQueue = ref([])
const messagesInTransit = ref([])
const logs = ref([])

const publishTopic = ref('sensors/temp')
const publishMessage = ref('25.5')
const publishQoS = ref('0')

const stats = ref({
  published: 0,
  delivered: 0
})

let msgId = 0
let logId = 0
let animationFrames = []

const activeSubscribers = computed(() => subscribers.value.filter(s => s.subscribed))

const canPublish = computed(() =>
  publishTopic.value.length > 0 && publishMessage.value.length > 0 && brokerActive.value
)

function selectPublisher(pub) {
  selectedPublisher.value = pub
  publishTopic.value = pub.topic
}

function toggleSubscription(sub) {
  sub.subscribed = !sub.subscribed
  addLog(sub.subscribed ? 'subscribe' : 'unsubscribe',
    `${sub.icon} ${sub.name} ${sub.subscribed ? 'abonné à' : 'désabonné de'} ${sub.filter}`)
}

function addLog(type, text) {
  const icons = {
    publish: '📤',
    deliver: '📥',
    subscribe: '✅',
    unsubscribe: '❌',
    broker: '📡'
  }
  logs.value.unshift({
    id: logId++,
    type,
    icon: icons[type] || '📝',
    text
  })
  if (logs.value.length > 6) logs.value.pop()
}

function topicMatches(topic, filter) {
  if (filter === '#') return true
  if (filter.endsWith('/#')) {
    const prefix = filter.slice(0, -2)
    return topic.startsWith(prefix)
  }
  if (filter.includes('+')) {
    const filterParts = filter.split('/')
    const topicParts = topic.split('/')
    if (filterParts.length !== topicParts.length) return false
    return filterParts.every((part, i) => part === '+' || part === topicParts[i])
  }
  return topic === filter
}

function publishMsg() {
  if (!canPublish.value) return

  const pub = selectedPublisher.value || publishers.value[0]
  const pubIdx = publishers.value.findIndex(p => p.id === pub.id)

  stats.value.published++
  addLog('publish', `${pub.icon} → ${publishTopic.value}: ${publishMessage.value}`)

  // Créer message en transit vers le broker
  const msg = {
    id: msgId++,
    x: 95,
    y: 65 + pubIdx * 55,
    phase: 'publish',
    icon: pub.icon,
    topic: publishTopic.value,
    qos: parseInt(publishQoS.value),
    targetSubs: []
  }

  messagesInTransit.value.push(msg)

  // Animation vers le broker
  animateMessage(msg, 95, 65 + pubIdx * 55, 180, 130, 'publish', () => {
    // Arrivée au broker
    addLog('broker', `Broker reçoit: ${publishTopic.value}`)

    // Trouver les subscribers correspondants
    const matchingSubs = activeSubscribers.value.filter(sub =>
      topicMatches(publishTopic.value, sub.filter)
    )

    if (matchingSubs.length === 0) {
      addLog('broker', 'Aucun subscriber correspondant')
      messagesInTransit.value = messagesInTransit.value.filter(m => m.id !== msg.id)
      return
    }

    // Distribuer aux subscribers
    matchingSubs.forEach((sub, idx) => {
      const subIdx = subscribers.value.findIndex(s => s.id === sub.id)
      const deliveryMsg = {
        id: msgId++,
        x: 280,
        y: 130,
        phase: 'deliver',
        icon: msg.icon,
        topic: publishTopic.value
      }

      setTimeout(() => {
        messagesInTransit.value.push(deliveryMsg)
        animateMessage(deliveryMsg, 280, 130, 356, 65 + subIdx * 55, 'deliver', () => {
          stats.value.delivered++
          addLog('deliver', `${sub.icon} reçoit: ${publishTopic.value}`)
          messagesInTransit.value = messagesInTransit.value.filter(m => m.id !== deliveryMsg.id)
        })
      }, idx * 150)
    })

    // Supprimer le message original
    messagesInTransit.value = messagesInTransit.value.filter(m => m.id !== msg.id)
  })
}

function animateMessage(msg, startX, startY, endX, endY, phase, onComplete) {
  const duration = 600
  const startTime = Date.now()

  function animate() {
    const elapsed = Date.now() - startTime
    const progress = Math.min(elapsed / duration, 1)

    // Easing
    const eased = 1 - Math.pow(1 - progress, 3)

    msg.x = startX + (endX - startX) * eased
    msg.y = startY + (endY - startY) * eased

    if (progress < 1) {
      const frameId = requestAnimationFrame(animate)
      animationFrames.push(frameId)
    } else {
      onComplete?.()
    }
  }

  animate()
}

function resetDemo() {
  animationFrames.forEach(id => cancelAnimationFrame(id))
  animationFrames = []

  publishers.value.forEach(p => p.active = true)
  subscribers.value[0].subscribed = true
  subscribers.value[1].subscribed = true
  subscribers.value[2].subscribed = false

  selectedPublisher.value = null
  messageQueue.value = []
  messagesInTransit.value = []
  logs.value = []
  stats.value = { published: 0, delivered: 0 }

  publishTopic.value = 'sensors/temp'
  publishMessage.value = '25.5'
  publishQoS.value = '0'

  addLog('broker', 'Système réinitialisé')
}

onUnmounted(() => {
  animationFrames.forEach(id => cancelAnimationFrame(id))
})
</script>

<style scoped>
.demo-container {
  font-family: 'Segoe UI', system-ui, sans-serif;
  padding: 4px;
  background: #fafafa;
  border-radius: 8px;
  font-size: 10px;
}

.main-container {
  display: flex;
  gap: 10px;
}

.graph-area {
  flex: 1;
  min-width: 460px;
}

.mqtt-svg {
  width: 100%;
  height: auto;
  max-height: 380px;
}

.controls {
  width: 170px;
  flex-shrink: 0;
}

.section {
  background: white;
  padding: 6px;
  border-radius: 6px;
  box-shadow: 0 1px 2px rgba(0,0,0,0.1);
  margin-bottom: 6px;
}

.section h4 {
  margin: 0 0 4px 0;
  font-size: 10px;
  color: #333;
  border-bottom: 1px solid #eee;
  padding-bottom: 3px;
}

.publish-form {
  display: flex;
  flex-direction: column;
  gap: 3px;
}

.publish-form label {
  font-size: 9px;
  color: #666;
  margin-top: 2px;
}

.publish-form input, .publish-form select {
  padding: 4px 6px;
  font-size: 9px;
  border: 1px solid #ddd;
  border-radius: 4px;
}

.publish-form input:focus, .publish-form select:focus {
  outline: none;
  border-color: #2196F3;
}

button {
  padding: 5px 8px;
  font-size: 10px;
  border: 1px solid #ddd;
  border-radius: 4px;
  background: white;
  cursor: pointer;
  transition: all 0.2s;
}

button:hover:not(:disabled) {
  background: #f0f0f0;
}

button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.publish-btn {
  background: #2196F3;
  color: white;
  border-color: #1976D2;
  margin-top: 4px;
}

.publish-btn:hover:not(:disabled) {
  background: #1976D2;
}

.reset-btn {
  width: 100%;
  background: #f5f5f5;
  color: #666;
}

.stat-row {
  display: flex;
  justify-content: space-between;
  padding: 2px 0;
  font-size: 9px;
}

.stat-value {
  font-weight: bold;
  color: #2196F3;
}

.log-area {
  max-height: 90px;
  overflow-y: auto;
}

.log-entry {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 2px 0;
  font-size: 8px;
  border-bottom: 1px solid #f5f5f5;
}

.log-entry.publish { color: #2196F3; }
.log-entry.deliver { color: #4CAF50; }
.log-entry.subscribe { color: #4CAF50; }
.log-entry.unsubscribe { color: #F44336; }
.log-entry.broker { color: #FF9800; }

.log-icon {
  font-size: 10px;
}

.log-text {
  flex: 1;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

/* SVG Styles */
.zone-title {
  font-size: 4.5px;
  font-weight: bold;
}

.broker-title {
  font-size: 5px;
  font-weight: bold;
  fill: #FF9800;
}

.broker-name {
  font-size: 4px;
  fill: #666;
}

.queue-label {
  font-size: 3px;
  fill: #999;
}

.queue-msg {
  font-size: 3px;
  fill: white;
  font-weight: bold;
}

.publisher-node, .subscriber-node {
  cursor: pointer;
  transition: transform 0.2s;
}

.publisher-node:hover, .subscriber-node:hover {
  transform: scale(1.03);
}

.node-icon {
  font-size: 5px;
}

.node-name {
  font-size: 3.5px;
  font-weight: bold;
}

.node-topic {
  font-size: 3px;
}

.transit-message {
  filter: drop-shadow(0 1px 2px rgba(0,0,0,0.3));
}

.transit-icon {
  font-size: 3px;
}

.status-led {
  filter: drop-shadow(0 0 3px rgba(76, 175, 80, 0.5));
}

.legend-text {
  font-size: 3px;
  fill: #666;
}
</style>
