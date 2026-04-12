# Puente — Narrow Bridge Simulation

A multithreaded C simulation of a one-lane bridge with real-time SDL2 visualization. Cars and ambulances arrive from both sides and must cross without collisions, controlled by one of three synchronization modes.

## Features

- Concurrent car threads with randomized arrival times (exponential distribution) and speeds
- Ambulance priority: ambulances bypass turn and semaphore restrictions, but do not corrupt the current control state
- Three control modes with distinct visual feedback
- Real-time SDL2 window showing the bridge, queues, and mode-specific UI
- Fully configurable via a plain-text config file

## Modes

| Mode | Description |
|------|-------------|
| `carnage` | No control. Cars enter as long as the bridge has capacity and traffic flows in one direction. |
| `semaforo` | A timed traffic light alternates which side may enter. Each side can have a different green duration. |
| `oficial` | A virtual officer manages turns. Each side has a car quota (`k_este`, `k_oeste`). When a side's quota is exhausted or its queue empties, the officer waits `segundos_espera` seconds before switching to the other side. |

## Building

**Dependencies**

```
sudo apt install libsdl2-dev libsdl2-ttf-dev
```

**Compile**

```
make
```

**Run**

```
make run
```

or

```
./puente
```

The simulation reads `puente.config` from the current directory.

## Configuration

All parameters live in `puente.config`. Example:

```ini
# Control mode: carnage | semaforo | oficial
modo = oficial

# Bridge length (number of simultaneous car slots)
longitudpuente = 8

# Total cars to generate per side
max_autos_este  = 25
max_autos_oeste = 25

# Ambulance probability per side (0-100)
porcentaje_ambulancias_este  = 10
porcentaje_ambulancias_oeste = 10

# Mean inter-arrival time in seconds (exponential distribution)
media_llegadas_este  = 1.5
media_llegadas_oeste = 2.0

# Car speed range (arbitrary units)
# Speed varies based of the average given, it will never be more than the max set and less than the mininum set
velocidad_promedio_este = 50
vel_min_este            = 30
vel_max_este            = 70

velocidad_promedio_oeste = 40
vel_min_oeste            = 25
vel_max_oeste            = 60

# Oficial mode: max cars per turn before switching
k_este   = 5
k_oeste  = 5

# Oficial mode: seconds officer waits on empty side before switching
segundos_espera = 2

# Semaforo mode: green duration per side in seconds
tiempo_semaforo_este  = 5
tiempo_semaforo_oeste = 5
```

## Project Structure

```
.
├── main.c          # Entry point, car generators, thread lifecycle
├── puente.c/h      # Bridge synchronization logic
├── auto.c/h        # Car thread
├── visual.c/h      # SDL2 rendering loop
├── config.c/h      # Config file parser
├── random_utils.c/h # Random number utilities
├── puente.config   # Runtime configuration
└── makefile
```

## Example Configurations

Here are three ready-to-use configurations, one per mode. Only the mode and speed-related values change between them — copy any of them into `puente.config` to try that mode.

**Carnage**
```ini
modo = carnage
longitudpuente = 8
max_autos_este  = 25
max_autos_oeste = 25
porcentaje_ambulancias_este  = 10
porcentaje_ambulancias_oeste = 10
media_llegadas_este  = 1.5
media_llegadas_oeste = 1.5
velocidad_promedio_este  = 60
vel_min_este             = 40
vel_max_este             = 80
velocidad_promedio_oeste = 60
vel_min_oeste            = 40
vel_max_oeste            = 80
k_este          = 5
k_oeste         = 5
segundos_espera = 2
tiempo_semaforo_este  = 5
tiempo_semaforo_oeste = 5
```

**Semaphore**
```ini
modo = semaforo
longitudpuente = 8
max_autos_este  = 25
max_autos_oeste = 25
porcentaje_ambulancias_este  = 10
porcentaje_ambulancias_oeste = 10
media_llegadas_este  = 1.5
media_llegadas_oeste = 1.5
velocidad_promedio_este  = 45
vel_min_este             = 30
vel_max_este             = 60
velocidad_promedio_oeste = 45
vel_min_oeste            = 30
vel_max_oeste            = 60
k_este          = 5
k_oeste         = 5
segundos_espera = 2
tiempo_semaforo_este  = 4
tiempo_semaforo_oeste = 6
```

**Police Officer**
```ini
modo = policia
longitudpuente = 8
max_autos_este  = 25
max_autos_oeste = 25
porcentaje_ambulancias_este  = 10
porcentaje_ambulancias_oeste = 10
media_llegadas_este  = 1.5
media_llegadas_oeste = 1.5
velocidad_promedio_este  = 50
vel_min_este             = 35
vel_max_este             = 70
velocidad_promedio_oeste = 50
vel_min_oeste            = 35
vel_max_oeste            = 70
k_este          = 4
k_oeste         = 6
segundos_espera = 3
tiempo_semaforo_este  = 5
tiempo_semaforo_oeste = 5
```

## Authors

Manfred Azofeifa Salazar  
Jose Adrián Herrera Salas
