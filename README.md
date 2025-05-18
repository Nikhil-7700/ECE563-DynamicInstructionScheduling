# Dynamic Instruction Scheduling Simulator

This repository implements a C++ simulator for modeling an out-of-order superscalar processor using dynamic instruction scheduling. This simulator mimics the behavior of a modern processor pipeline, including instruction fetch, decode, rename, issue, execute, writeback, and retire stages.

## Project Summary

The simulator evaluates the effect of processor configuration parameters such as **Issue Queue size**, **Reorder Buffer size**, and **Superscalar Width** on performance. The main goal is to explore **Instructions Per Cycle (IPC)** under different configurations using real instruction traces.

- **Course**: ECE 563 – Microprocessor Architecture
- **Project**: Project #3 – Dynamic Instruction Scheduling
- **Author**: Padmanabha Nikhil Bhimavarapu
- **Language**: C++
- **Architecture Modeled**: Out-of-order execution with perfect memory and branch prediction

## Key Concepts

- Superscalar width: Number of instructions fetched, decoded, issued, and retired per cycle
- Issue Queue (IQ): Buffer for instructions waiting for their operands
- Reorder Buffer (ROB): Ensures in-order retirement
- Register Renaming: Avoids WAR/WAW hazards
- Pipeline stages: FE -> DE -> RN -> RR -> DI -> IS -> EX -> WB -> RT

## How to Run

### Command-Line Syntax

```bash
./sim <ROB_SIZE> <IQ_SIZE> <WIDTH> <tracefile>
