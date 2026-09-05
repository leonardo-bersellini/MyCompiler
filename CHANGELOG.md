## ChangeLog

 This file keeps track of all the main changes for every version of the bismuth compiler project.

 > **Note:** early version numbers in this changelog were not assigned following
> correct semantic versioning rules. Version numbers here have been retroactively
> corrected and may not match the version referenced in the original commit —
> for versions below 1.7.0, this changelog should be treated as the source of truth.


### Convention

Each entry starts with the version, formatted as `V x.x.x`, followed by the
committer in `<@username>` format and the date of the change.

Below the version header, each entry lists a set of fields in column form,
each written as `field_name:` followed by its value.         

---
 
 ### Bismuth Versions

 ---

 **V 1.8.3**  &ensp; <@leonardo-bersellini> &emsp; 05 . 09 . 2026

 `commit:` Added Changelog *-o- ()*
 <br>
 `scope:` All
 <br>
 `features:` implements this changelog to the project

 ---

 **V 1.8.2**  &ensp; <@leonardo-bersellini> &emsp; 05 . 09 . 2026

 `commit:` Tests for Bismuth *-o- (1bb022c)*
 <br>
 `scope:` Tests
 <br>
 `features:` added tests for all bismuth's components.

 ---

 **V 1.8.1**  &ensp; <@leonardo-bersellini> &emsp; 05 . 09 . 2026

 `commit:` Fixed void keyword *-o- (2d5d172)*
 <br>
 `scope:` Parser, Codegen
 <br>
 `features:` fixed an old bug preventing the void keyword from being used.

 ---

 **V 1.8.0**  &ensp; <@leonardo-bersellini> &emsp; 04 . 09 . 2026

 `commit:` Arrays: indexed access and parameters *-o- (bcfe667)*
 <br>
 `scope:` Parser, Semantics, Codegen
 <br>
 `features:` finished arrays implementation adding access to a value inside an array.

 ---

 **V 1.7.8**  &ensp; <@leonardo-bersellini> &emsp; 04 . 09 . 2026

 `commit:` Expression-based Assignments *-o- (cbfe3c1)*
 <br>
 `scope:` Parser, Semantics, Codegen
 <br>
 `features:` refactored expression. now an expression works on an lvalue and rvalue.

 ---

 **V 1.7.6**  &ensp; <@leonardo-bersellini> &emsp; 02 . 09 . 2026

 `commit:` Refactored type structure *-o- (3eb8a50)*
 <br>
 `scope:` Types
 <br>
 `features:` refator of types structure for complex types such as arrays, made implementing a variadic overloaded visitor.

 ---

 **V 1.7.4**  &ensp; <@leonardo-bersellini> &emsp; 01 . 09 . 2026

 `commit:` Arrays and new Primitive Types *-o- (2aaa228)*
 <br>
 `scope:` All
 <br>
 `features:` added first part of static-arrays implementation

 ---

 **V 1.7.1**  &ensp; <@leonardo-bersellini> &emsp; 31 . 08 . 2026

 `commit:` Implemented short utility flags *-o- (94e8b66)*
 <br>
 `scope:` Compiler Driver
 <br>
 `features:` added short flags as identifier for utility flags, updated cliparser logic

 ---

 **V 1.7.0**  &ensp; <@leonardo-bersellini> &emsp; 30 . 08 . 2026

 `commit:` Error Recovery *-o- (b2350c2)*
 <br>
 `scope:` Parser
 <br>
 `features:` recovery strategies for errors collected during parser execution

 ---
 
 **V 1.6.0**  &ensp; <@leonardo-bersellini> &emsp; 29 . 08 . 2026

 `commit:` Const keyword *-o- (a4fba91)*
 <br>
 `scope:` All
 <br>
 `features:` const variables and declarations

 ---
 
 **V 1.5.0**  &ensp; <@leonardo-bersellini> &emsp; 28 . 08 . 2026

 `commit:` Switch Instruction *-o- (95d327f)*
 <br>
 `scope:` All
 <br>
 `features:` added a new instrcution for bismuth language: switch

 ---
 
 **V 1.4.4**  &ensp; <@leonardo-bersellini> &emsp; 25 . 08 . 2026

 `commit:` Fixed codegen bugs *-o- (b51c2c4)*
 <br>
 `scope:` Codegen
 <br>
 `features:` minor bugs fixed

 ---
 
 **V 1.4.3**  &ensp; <@leonardo-bersellini> &emsp; 25 . 08 . 2026

 `commit:` Codegen: scoped symbol table *-o- (46ce210)*
 <br>
 `scope:` Codegen
 <br>
 `features:` added a symbol table for correct generation of shadowed variables

 ---

 **V 1.2.0**  &ensp; <@leonardo-bersellini> &emsp; 25 . 08 . 2026

 `commit:` Implemented top-level statements control *-o- (89da22e)*
 <br>
 `scope:` Semantics
 <br>
 `features:` added top-level rules 

 ---

 **V 1.1.2**  &ensp; <@leonardo-bersellini> &emsp; 24 . 08 . 2026

 `commit:` Ansi console and messages *-o- (6d0288c)*
 <br>
 `scope:` Utils
 <br>
 `features:` compilation messages written using ansi colors

 ---

 **V 1.0.2**  &ensp; <@leonardo-bersellini> &emsp; 20 . 08 . 2026

 `commit:` refactor: migrate from Qt to standard C++ *-o- (c286a78)*
 <br>
 `scope:` All
 <br>
 `features:` all the code is written using std c++
 
 ---

 **V 0.1.0**  &ensp; <@leonardo-bersellini> &emsp; 20 . 07 . 2026
 
 `commit:` Linker *-o- (09a75da)*
 <br>
 `scope:` Codegen
 <br>
 `features:` implemented a linker for executable files generation