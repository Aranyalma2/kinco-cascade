# Kinco HMI cascade macro

# Register and Macro Descriptions

## Registers
| Register | Description |
|----------|------------|
| LW1000 | Hűtés/Fűtés (0/1) |
| LW1001 | Hűtés/Fűtés alapjel (°C) |
| LW1002 | Hűtés/Fűtés mért hőmérséklet (°C) |
| LW1003 | Hűtés/Fűtés hőmérséklet hiszterézik (°C) |
| LW1004 | HMV minimum hőmérséklet (°C) |
| LW1005 | HMV fűtés gép fenntartás igény esetén (legalább) (db) |
| LW1006 | HMV fűtés gép fenntartás igény esetén (maximum) (db) |
| LW1007 | HMV cél hőmérséklet (°C) |

### Group 1 Registers
| Register | Description |
|----------|------------|
| LW1100 | 1 - Engedélyezés (0/1) |
| LW1101 | 1 - HMV fűtés visszacsatolás (0/1) |
| LW1102 | 1 - HMV tartály hőmérséklet (°C) |
| LW1103 | 1 - Hiba |
| LW1104 | 1 - Üzemidő (LSB) |
| LW1105 | 1 - Üzemidő (MSB) |
| LW1106 | 1 - Hűtés/Fűtés indítás (0/1) |
| LW1107 | 1 - HMV indítás (0/1) |

### Group 2 Registers
| Register | Description |
|----------|------------|
| LW1108 | 2 - Engedélyezés (0/1) |
| LW1109 | 2 - HMV fűtés visszacsatolás (0/1) |
| LW1110 | 2 - HMV tartály hőmérséklet (°C) |
| LW1111 | 2 - Hiba |
| LW1112 | 2 - Üzemidő (LSB) |
| LW1113 | 2 - Üzemidő (MSB) |
| LW1114 | 2 - Hűtés/Fűtés indítás (0/1) |
| LW1115 | 2 - HMV indítás (0/1) |

### Group 3 Registers
| Register | Description |
|----------|------------|
| LW1116 | 3 - Engedélyezés (0/1) |
| LW1117 | 3 - HMV fűtés visszacsatolás (0/1) |
| LW1118 | 3 - HMV tartály hőmérséklet (°C) |
| LW1119 | 3 - Hiba |
| LW1120 | 3 - Üzemidő (LSB) |
| LW1121 | 3 - Üzemidő (MSB) |
| LW1122 | 3 - Hűtés/Fűtés indítás (0/1) |
| LW1123 | 3 - HMV indítás (0/1) |

### Group 4 Registers
| Register | Description |
|----------|------------|
| LW1124 | 4 - Engedélyezés (0/1) |
| LW1125 | 4 - HMV fűtés visszacsatolás (0/1) |
| LW1126 | 4 - HMV tartály hőmérséklet (°C) |
| LW1127 | 4 - Hiba |
| LW1128 | 4 - Üzemidő (LSB) |
| LW1129 | 4 - Üzemidő (MSB) |
| LW1130 | 4 - Hűtés/Fűtés indítás (0/1) |
| LW1131 | 4 - HMV indítás (0/1) |

## Macros
| Macro | Description |
|-------|------------|
| cascad.c | Vezérlési logika, futtatásra egyszeri változás |
| runtime.c | Üzemóra számláló, futtatásra számol (ajánlott 1 percenként 1x) |

---
### Notes
- This table provides an overview of system registers and macros.
- Each register is grouped based on function and numbering scheme.
- Macros define operational logic and runtime counters.

