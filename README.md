# adc_soc_cputimers.c

## Title: ADC Triggering via CPU Timer for F28P65x

---

### Description

This example configures CPU Timer0 to periodically trigger the ADC.

---

### External Connections

- A0 should be connected to the signal to convert.

---

### Watch Variables

- **AdcaResults**:  
  A sequence of analog-to-digital conversion samples from pin A0.  
  The time between samples is determined based on the period of the CPU timer.

---
