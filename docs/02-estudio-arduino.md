# Selección de Arduino: Comparativa

| Característica | Arduino Mega 2560 | Arduino Micro | Arduino Leonardo | Arduino Nano |
|---|---|---|---|---|
| Precio | 25-40 | 15-25 | 20-30 | 10-15 |
| USB MIDI Nativo | NO (requiere firmware) | SÍ (ATmega32U4) | SÍ (ATmega32U4) | NO |
| Pines digitales | 54 | 20 | 20 | 22 |
| Pines analógicos | 16 | 12 | 12 | 8 |
| Tamaño | Grande | Muy pequeño | Pequeño | Pequeño |
| Facilidad configuración | Media | Alta | Alta | Media |
| Ideal para MIDI | Mejor con DFU flashing | Mejor opción | Alternativa buena | Requiere firmware |

### Decisión final: **Arduino Micro**

**Razones:**
1. Soporte nativo USB MIDI (chipset ATmega32U4)
2. Plug-and-play sin configuración adicional de firmware
3. Tamaño compacto ideal para caja de pedalera
4. Suficientes pines para 10 footswitches + 5 encoders + 8 LEDs
5. Mejor relación precio/funcionalidad

**Alternativa:** Arduino Leonardo (casi idéntico, pero ligeramente más grande)
