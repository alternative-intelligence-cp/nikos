# NIKOS

NIKOS (New Inference Kernel for Open Static Analyzers) is a modernized fork of NASA's IKOS framework. 
It is specifically designed to update the core IKOS infrastructure to natively support the LLVM 20 toolchain.

## Purpose
The primary objective of NIKOS is to integrate the power of Abstract Interpretation into the Nitpick verification ecosystem. 
By upgrading the LLVM frontend from LLVM 14 to LLVM 20, NIKOS resolves critical architectural bottlenecks, including the transition to opaque pointers and modern debug information records.

## License
NIKOS is built upon IKOS, which was originally developed by NASA and released under the NASA Open Source Agreement (NOSA) version 1.3. 
Modifications and new code in NIKOS remain open-source and compatible with these terms.

## Status
**Cycle 0.0 Completed:** Environment Provisioning, CMake configurations, and Initial Build Baseline are established. We are currently migrating the codebase to LLVM 20 and modern C++17.
