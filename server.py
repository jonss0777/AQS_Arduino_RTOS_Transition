from fastapi import FastAPI, Request
from pydantic import BaseModel
from typing import List

app = FastAPI(title="ESP32 Sensor")

# Define Pydantic models to validate the incoming JSON payload
class SensorReading(BaseModel):
    sensorA: str
    valA: float
    sensorB: str
    valB: float
    sensorC: str
    valC: float

class SensorBatch(BaseModel):
    macAddress: str
    readings: List[SensorReading]
    
class HeartbeatPayload(BaseModel):
    macAddress: str
    uptimeSeconds: int
    
@app.get("/")
async def demo():
    return {
        "msg":"Hello World"
    }
    
@app.post("/api/heartbeat")
async def receive_heartbeat(hb: HeartbeatPayload):
    print(f"[Heartbeat] Device {hb.macAddress} is alive! Uptime: {hb.uptimeSeconds}s")
    return {"status": "alive"}

@app.post("/api/sensor-reading")
async def receive_sensor_batch(reading: SensorBatch):
    print("\n--- Received Sensor Batch ---")
    print(f"ESP32 MAC Address: {reading.macAddress}")
   # print(f"Total Readings: {len()}")
    #print(reading)
    for idx, reading in enumerate(reading.readings[0:3], start=1):
        print(f"  [{idx}] Sensor ID: {reading.sensorA} | Value: {reading.valA}")
        print(f"  [{idx}] Sensor ID: {reading.sensorB} | Value: {reading.valB}")
        print(f"  [{idx}] Sensor ID: {reading.sensorC} | Value: {reading.valC}")
        
    # Return a success response back to the ESP32
    return {
        "status": "success", 
        "message": "Batch received successfully",
    
    }

if __name__ == "__main__":
    import uvicorn
    # Runs on port 3000, accessible from any device on your local network (0.0.0.0)
    uvicorn.run(app, host="0.0.0.0", port=3002)
