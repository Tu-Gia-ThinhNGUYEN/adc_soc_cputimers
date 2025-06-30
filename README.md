//#############################################################################
//
// FILE:   adc_soc_cputimers.c
//
// TITLE:  ADC triggering via CPU timer for f28p65x.
//
//! \addtogroup bitfield_example_list
//! <h1> ADC CPU Timer Triggering </h1>
//!
//! This example configures CPU Timer0 to periodically trigger the ADC.
//!
//! \b External \b Connections \n
//!  - A0 should be connected to signals to convert
//!
//! \b Watch \b Variables \n
//!  - \b AdcaResults \b: A sequence of analog-to-digital conversion samples from
//! pin A0. The time between samples is determined based on the period
//! of the ePWM timer.
//!
//
//#############################################################################