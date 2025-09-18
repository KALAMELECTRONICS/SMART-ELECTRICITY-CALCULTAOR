#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>


const char* WIFI_SSID = "Kalam_004";
const char* WIFI_PASS = "kalam@2025";
#define USE_AP_MODE false
const char* AP_SSID = "SmartPower-ESP32";
const char* AP_PASS = "12345678";


const float DEMO_VOLT = 12.0f;     
const float MAX_FAKE_W1 = 120.0f;   
const float MAX_FAKE_W2 = 120.0f;   


float thrW_room1_default = 60.0f;
float thrW_room2_default = 60.0f;


const bool RELAY_ACTIVE_LOW = true;


const int PIN_POT1   = 32;  
const int PIN_POT2   = 33;  
const int PIN_RELAY1 = 19;
const int PIN_RELAY2 = 18;


WebServer server(80);
Preferences prefs;

float Watts1 = 0, Watts2 = 0;
float Amps1  = 0, Amps2  = 0;
float thrW_room1 = 0, thrW_room2 = 0;
bool relay1_on = true, relay2_on = true;

inline int relayOnLevel()  { return RELAY_ACTIVE_LOW ? LOW  : HIGH; }
inline int relayOffLevel() { return RELAY_ACTIVE_LOW ? HIGH : LOW; }
void setRelay(int pin, bool on){ digitalWrite(pin, on ? relayOnLevel() : relayOffLevel()); }


float potToWatts(int pin, float maxW, bool invert=false){
  int raw = analogRead(pin);             
  float frac = raw / 4095.0f;            
  if (invert) frac = 1.0f - frac;
  return frac * maxW;
}


String htmlIndex(){
  String page = R"HTML(
<!doctype html>
<html><head>
<meta name=viewport content="width=device-width,initial-scale=1" />
<title>Smart Electricity Monitor (2 Pots)</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap');

:root {
  --primary: #6366F1;
  --primary-dark: #4F46E5;
  --secondary: #10B981;
  --danger: #EF4444;
  --warning: #F59E0B;
  --bg-primary: #0F172A;
  --bg-secondary: #1E293B;
  --bg-card: #334155;
  --text-primary: #F8FAFC;
  --text-secondary: #CBD5E1;
  --text-muted: #64748B;
  --border: #475569;
  --shadow: rgba(0, 0, 0, 0.25);
  --glow: rgba(99, 102, 241, 0.3);
}

* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  font-family: 'Inter', system-ui, -apple-system, sans-serif;
  background: linear-gradient(135deg, var(--bg-primary) 0%, #1a1a2e 100%);
  color: var(--text-primary);
  min-height: 100vh;
  position: relative;
  overflow-x: hidden;
}

/* Animated background particles */
body::before {
  content: '';
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: radial-gradient(circle at 20% 50%, rgba(99, 102, 241, 0.1) 0%, transparent 50%),
              radial-gradient(circle at 80% 20%, rgba(16, 185, 129, 0.1) 0%, transparent 50%),
              radial-gradient(circle at 40% 80%, rgba(245, 158, 11, 0.1) 0%, transparent 50%);
  animation: float 20s ease-in-out infinite;
  pointer-events: none;
  z-index: -1;
}

@keyframes float {
  0%, 100% { transform: translate(0, 0) rotate(0deg); }
  33% { transform: translate(30px, -30px) rotate(120deg); }
  66% { transform: translate(-20px, 20px) rotate(240deg); }
}

.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 2rem;
  position: relative;
  z-index: 1;
}

h1 {
  text-align: center;
  font-size: clamp(1.5rem, 4vw, 2.5rem);
  font-weight: 700;
  background: linear-gradient(135deg, var(--primary), var(--secondary));
  background-clip: text;
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  margin-bottom: 3rem;
  text-shadow: 0 0 30px var(--glow);
  animation: pulse-glow 3s ease-in-out infinite alternate;
}

@keyframes pulse-glow {
  from { filter: brightness(1); }
  to { filter: brightness(1.2); }
}

.grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(350px, 1fr));
  gap: 2rem;
  margin-bottom: 2rem;
  max-width: 1200px; /* Adjust based on your design */
  margin-left: auto;
  margin-right: auto;
  justify-items: center; /* This centers cards 1 & 2 in their columns */
}

.card {
  background: linear-gradient(135deg, var(--bg-card) 0%, var(--bg-secondary) 100%);
  backdrop-filter: blur(20px);
  border: 1px solid var(--border);
  padding: 2rem;
  border-radius: 20px;
  box-shadow: 
    0 20px 25px -5px var(--shadow),
    0 10px 10px -5px var(--shadow),
    inset 0 1px 0 rgba(255, 255, 255, 0.1);
  position: relative;
  overflow: hidden;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  width: 100%;
  max-width: 500px; /* Prevents cards from getting too wide */
}

.card::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 3px;
  background: linear-gradient(90deg, var(--primary), var(--secondary));
  border-radius: 20px 20px 0 0;
}

.card:hover {
  transform: translateY(-8px);
  box-shadow: 
    0 32px 40px -12px var(--shadow),
    0 20px 25px -5px rgba(99, 102, 241, 0.2),
    inset 0 1px 0 rgba(255, 255, 255, 0.2);
}

.card3 {
  grid-column: 1 / -1; /* Spans across both columns */
  background: linear-gradient(135deg, var(--bg-secondary) 0%, var(--bg-card) 100%);
  max-width: 500px; /* Same as other cards for consistency */
  justify-self: center; /* Centers the card in the spanned area */
}

.card3::before {
  background: linear-gradient(90deg, var(--warning), var(--danger));
}

.row {
  display: flex;
  align-items: center;
  gap: 1rem;
  flex-wrap: wrap;
  margin-bottom: 1rem;
}

.row h3 {
  font-size: 1.5rem;
  font-weight: 600;
  color: var(--text-primary);
  margin: 0;
}

.val {
  font-size: 3rem;
  font-weight: 700;
  margin: 1rem 0;
  background: linear-gradient(135deg, #FFFFFF, var(--text-secondary));
  background-clip: text;
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  text-shadow: 0 0 20px rgba(255, 255, 255, 0.3);
}

.badge {
  display: inline-flex;
  align-items: center;
  padding: 0.5rem 1rem;
  border-radius: 50px;
  font-size: 0.75rem;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  transition: all 0.3s ease;
  box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.3);
}

.badge.on {
  background: linear-gradient(135deg, var(--secondary), #059669);
  color: white;
  box-shadow: 0 0 20px rgba(16, 185, 129, 0.4);
}

.badge.off {
  background: linear-gradient(135deg, var(--danger), #DC2626);
  color: white;
  box-shadow: 0 0 20px rgba(239, 68, 68, 0.4);
}

.btn {
  background: linear-gradient(135deg, var(--primary), var(--primary-dark));
  color: white;
  border: none;
  padding: 0.75rem 1.5rem;
  border-radius: 12px;
  cursor: pointer;
  font-weight: 500;
  font-size: 0.875rem;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: 
    0 4px 6px -1px rgba(99, 102, 241, 0.3),
    0 2px 4px -1px rgba(99, 102, 241, 0.2);
  position: relative;
  overflow: hidden;
}

.btn::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.5s;
}

.btn:hover {
  background: linear-gradient(135deg, #5B21B6, var(--primary));
  transform: translateY(-2px);
  box-shadow: 
    0 10px 15px -3px rgba(99, 102, 241, 0.4),
    0 4px 6px -2px rgba(99, 102, 241, 0.3);
}

.btn:hover::before {
  left: 100%;
}

.btn:active {
  transform: translateY(0);
}

label {
  font-size: 0.875rem;
  color: var(--text-secondary);
  font-weight: 500;
}

input[type=number] {
  background: rgba(255, 255, 255, 0.1);
  border: 2px solid var(--border);
  color: var(--text-primary);
  padding: 0.75rem;
  border-radius: 12px;
  font-size: 0.875rem;
  font-weight: 500;
  transition: all 0.3s ease;
  backdrop-filter: blur(10px);
  width: 120px;
}

input[type=number]:focus {
  outline: none;
  border-color: var(--primary);
  box-shadow: 0 0 0 3px rgba(99, 102, 241, 0.2);
  background: rgba(255, 255, 255, 0.15);
}

.small {
  font-size: 0.875rem;
  color: var(--text-muted);
  line-height: 1.5;
  margin-bottom: 0.5rem;
}

.on {
  color: var(--secondary);
  font-weight: 600;
  text-shadow: 0 0 10px rgba(16, 185, 129, 0.5);
}

.off {
  color: var(--danger);
  font-weight: 600;
  text-shadow: 0 0 10px rgba(239, 68, 68, 0.5);
}

hr {
  border: none;
  height: 1px;
  background: linear-gradient(90deg, transparent, var(--border), transparent);
  margin: 1.5rem 0;
}

/* Responsive Design */
@media (max-width: 768px) {
  .container {
    padding: 1rem;
  }
  
  .grid {
    grid-template-columns: 1fr;
    gap: 1.5rem;
  }
  
  .card {
    padding: 1.5rem;
  }
  
  .val {
    font-size: 2.5rem;
  }
  
  .row {
    flex-direction: column;
    align-items: flex-start;
    gap: 0.75rem;
  }
}

/* Loading animation for values */
@keyframes shimmer {
  0% { opacity: 0.6; }
  50% { opacity: 1; }
  100% { opacity: 0.6; }
}

.val span {
  animation: shimmer 2s ease-in-out infinite;
}
</style>
</head>

<body>
<div class="container">
  <h1>SMART ELECTRICITY MONITORING USING ESP32</h1>

  <div class="grid">
    <div class="card">
      <div class="row">
        <h3>ROOM 1</h3>
        <span id="r1state" class="badge">…</span>
      </div>
      <div class="val"><span id="w1">0</span> W</div>
      <div class="small">I ≈ <span id="i1">0</span> A @ 12V</div>
      <hr/>
      <div class="row">
        <label>Threshold (W)</label>
        <input id="thr1" type="number" step="1" min="1"/>
        <button class="btn" onclick="save()">Save</button>
      </div>
    </div>

    <div class="card">
      <div class="row">
        <h3>ROOM 2</h3>
        <span id="r2state" class="badge">…</span>
      </div>
      <div class="val"><span id="w2">0</span> W</div>
      <div class="small">I ≈ <span id="i2">0</span> A @ 12V</div>
      <hr/>
      <div class="row">
        <label>Threshold (W)</label>
        <input id="thr2" type="number" step="1" min="1"/>
        <button class="btn" onclick="save()">Save</button>
      </div>
    </div>

    <div class="card card3">
      <h3>SYSTEM</h3>
      <div class="small">Turn either pot → watts increase. Relays trip when Watts ≥ Threshold.</div>
      <div class="small">Relay1: <span id="rel1">…</span> | Relay2: <span id="rel2">…</span></div>
      <div class="small">IP: <span id="ip">…</span></div>
      <hr/>
      <div class="row">
        <button class="btn" onclick="resetRelays()">Reset Relays</button>
        <button class="btn" onclick="toggleFreeze()" id="freezeBtn">⏸ Freeze</button>
      </div>
    </div>
  </div>
</div>

<script>
let frozen = false;
let intervalId = null;

async function fetchData(){
  if (frozen) return;
  try{
    const r=await fetch('/api'); const j=await r.json();
    w1.textContent=j.w1.toFixed(0); w2.textContent=j.w2.toFixed(0);
    i1.textContent=j.i1.toFixed(2); i2.textContent=j.i2.toFixed(2);
    if (!thr1.matches(':focus')) thr1.value=j.t1.toFixed(0);
    if (!thr2.matches(':focus')) thr2.value=j.t2.toFixed(0);
    rel1.textContent=j.rel1?'ON':'OFF'; rel2.textContent=j.rel2?'ON':'OFF';
    rel1.className=j.rel1?'on':'off'; rel2.className=j.rel2?'on':'off';
    r1state.textContent=j.w1>=j.t1?'TRIPPED':'OK'; r1state.className='badge '+(j.w1>=j.t1?'off':'on');
    r2state.textContent=j.w2>=j.t2?'TRIPPED':'OK'; r2state.className='badge '+(j.w2>=j.t2?'off':'on');
    ip.textContent=j.ip;
  }catch(e){console.log(e)}
}

async function save(){ 
  await fetch(`/set?t1=${thr1.value}&t2=${thr2.value}`);
  fetchData(); 
}

async function resetRelays(){ 
  await fetch('/reset'); 
  fetchData(); 
}

function toggleFreeze(){
  frozen = !frozen;
  freezeBtn.textContent = frozen ? "▶ Resume" : "⏸ Freeze";
}

intervalId = setInterval(fetchData, 500); 
fetchData();
</script>
</body>
</html>
)HTML";
  return page;
}

void handleIndex(){ server.send(200, "text/html", htmlIndex()); }
void handleAPI(){
  String ip = WiFi.isConnected()? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  String j="{";
  j += "\"w1\":" + String(Watts1,0) + ",";
  j += "\"w2\":" + String(Watts2,0) + ",";
  j += "\"i1\":" + String(Amps1,2) + ",";
  j += "\"i2\":" + String(Amps2,2) + ",";
  j += "\"t1\":" + String(thrW_room1,0) + ",";
  j += "\"t2\":" + String(thrW_room2,0) + ",";
  j += "\"rel1\":" + String(relay1_on?"true":"false") + ",";
  j += "\"rel2\":" + String(relay2_on?"true":"false") + ",";
  j += "\"ip\":\"" + ip + "\"";
  j += "}";
  server.send(200, "application/json", j);
}
void handleSet(){
  if (server.hasArg("t1")) thrW_room1 = server.arg("t1").toFloat();
  if (server.hasArg("t2")) thrW_room2 = server.arg("t2").toFloat();
  prefs.putFloat("thr1", thrW_room1);
  prefs.putFloat("thr2", thrW_room2);
  server.send(200, "text/plain", "OK");
}
void handleReset(){
  relay1_on = true; setRelay(PIN_RELAY1, relay1_on);
  relay2_on = true; setRelay(PIN_RELAY2, relay2_on);
  server.send(200, "text/plain", "OK");
}


inline void adcSetup(int pin){ analogSetPinAttenuation(pin, ADC_11db); }
void setup(){
  Serial.begin(115200);
  pinMode(PIN_RELAY1, OUTPUT); pinMode(PIN_RELAY2, OUTPUT);
  setRelay(PIN_RELAY1, true); setRelay(PIN_RELAY2, true);
  analogReadResolution(12); adcSetup(PIN_POT1); adcSetup(PIN_POT2);

  prefs.begin("smartpower", false);
  thrW_room1 = prefs.getFloat("thr1", thrW_room1_default);
  thrW_room2 = prefs.getFloat("thr2", thrW_room2_default);

  if (USE_AP_MODE){
    WiFi.mode(WIFI_AP); WiFi.softAP(AP_SSID, AP_PASS);
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  } else {
    WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting");
    uint32_t t0=millis();
    while (WiFi.status()!=WL_CONNECTED && millis()-t0<12000){ delay(300); Serial.print("."); }
    if (WiFi.status()==WL_CONNECTED) Serial.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());
    else { Serial.println("\nWiFi failed → AP mode"); WiFi.mode(WIFI_AP); WiFi.softAP(AP_SSID, AP_PASS); Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str()); }
  }
  server.on("/", handleIndex);
  server.on("/api", handleAPI);
  server.on("/set", handleSet);
  server.on("/reset", handleReset);
  server.begin();
}

void loop(){
  server.handleClient();


  Watts1 = potToWatts(PIN_POT1, MAX_FAKE_W1, /*invert=*/false);
  Watts2 = potToWatts(PIN_POT2, MAX_FAKE_W2, /*invert=*/false);

  Amps1 = Watts1 / DEMO_VOLT;
  Amps2 = Watts2 / DEMO_VOLT;


  if (Watts1 >= thrW_room1) relay1_on = false;
  if (Watts2 >= thrW_room2) relay2_on = false;
  setRelay(PIN_RELAY1, relay1_on);
  setRelay(PIN_RELAY2, relay2_on);

  static uint32_t tLog=0; if (millis()-tLog>800){ tLog=millis();
    Serial.printf("R1: W=%.0f Thr=%.0f [%s] | R2: W=%.0f Thr=%.0f [%s]\n",
      Watts1, thrW_room1, relay1_on?"ON":"OFF",
      Watts2, thrW_room2, relay2_on?"ON":"OFF");
  }

}
