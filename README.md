:doctype: article
:description: A logging, profiling, and testing library for C and C++.
:repository-url: https://github.com/giopler/giopler-cpp
:license-type: MIT License (MIT)
:author: Giopler
:source-highlighter: rouge
:rouge-style: github
:toc: preamble

:icons: font
:icon-set: fas
:check: icon:check[set=fas,size=1x,role=green]
:cross: icon:times[set=fas,size=1x,role=red]

= giopler

A logging, profiling, and testing library for C and C++.

WARNING: This is very much a work in progress right now.
But development is moving quickly!

== About Me

¡Hola! I'm a C and C++ computer programmer with leadership experience.
My work usually focuses in the areas of software performance optimization
and functional programming.
You can find me on https://www.linkedin.com/in/creyes123/[LinkedIn].
You can also reach me directly via mailto:creyes123@yahoo.com[Email].

== Features

* Integrated logging, testing, and profiling library
* C and C++ support
* Can be completely turned off to have near zero runtime overhead
* Send recorded data to multiple destinations simultaneously
* Can record data to stdout, stderr, Linux syslog, and files
* Filter output data per device
* output format is either CSV, space-separated, or JSON
* profiling function counts are exact (not sampling)
* data is saved using structure, facilitating further processing

== Build Modes

.Build Modes
[cols="4,1,6"]
|===
|Name|Abbrev|Description

|GIOPLER_BUILD_MODE_OFF
|Off
|turn off with near zero runtime cost

|GIOPLER_BUILD_MODE_DEV
|Dev
|development and debugging

|GIOPLER_BUILD_MODE_TEST
|Test
|run unit tests

|GIOPLER_BUILD_MODE_PROF
|Prof
|improve performance

|GIOPLER_BUILD_MODE_QA
|Qa
|used by quality assurance team

|GIOPLER_BUILD_MODE_PROD
|Prod
|production deployments
|===

== Functionality Availability

.Functionality Enabled by Build Mode
[cols="3,1,1,1,1,1,1",frame=ends,grid=rows]
|===
|_Contract API_   |Off      |Dev      |Test     |Prof     |QA       |Prod
|dev::argument   ^|{cross} ^|{check} ^|{check} ^|{cross} ^|{check} ^|{cross}
|dev::expect     ^|{cross} ^|{check} ^|{check} ^|{cross} ^|{check} ^|{cross}
|dev::confirm    ^|{cross} ^|{check} ^|{check} ^|{cross} ^|{check} ^|{cross}
|dev::Invariant  ^|{cross} ^|{check} ^|{check} ^|{cross} ^|{check} ^|{cross}
|dev::Ensure     ^|{cross} ^|{check} ^|{check} ^|{cross} ^|{check} ^|{cross}
|prod::certify   ^|{cross} ^|{check} ^|{check} ^|{check} ^|{check} ^|{check}
7+|
|_Tracing API_    |Off      |Dev      |Test     |Prof     |QA       |Prod
|dev::line       ^|{cross} ^|{check} ^|{check} ^|{cross} ^|{cross} ^|{cross}
|dev::breakpoint ^|{cross} ^|{check} ^|{check} ^|{cross} ^|{cross} ^|{cross}
|prod::branch    ^|{cross} ^|{check} ^|{check} ^|{check} ^|{check} ^|{check}
7+|
|_Logging API_    |Off      |Dev      |Test     |Prof     |QA       |Prod
|dev::warning    ^|{cross} ^|{check} ^|{check} ^|{cross} ^|{cross} ^|{cross}
|prod::error     ^|{cross} ^|{check} ^|{check} ^|{check} ^|{check} ^|{check}
|prod::message   ^|{cross} ^|{check} ^|{check} ^|{check} ^|{check} ^|{check}
7+|
|_Profiling API_  |Off      |Dev      |Test     |Prof     |QA       |Prod
|prof::Function  ^|{cross} ^|{cross} ^|{cross} ^|{check} ^|{cross} ^|{cross}
|prof::Block     ^|{cross} ^|{cross} ^|{cross} ^|{check} ^|{cross} ^|{cross}
7+|
|_Testing API_    |Off      |Dev      |Test     |Prof     |QA       |Prod
|test::test      ^|{cross} ^|{cross} ^|{check} ^|{cross} ^|{cross} ^|{cross}
|test::benchmark ^|{cross} ^|{cross} ^|{check} ^|{cross} ^|{cross} ^|{cross}
|===

== Data Dictionary

.Data Dictionary
[cols="1,1,3"]
|===
|Label|Data Type|Description

|Timestamp
|interval
|ISO-8601

|Timestamp_sec
|interval
|Linux time, seconds

|ProcessId
|nominal
|integer from system

|ThreadId
|nominal
|integer from system

|FileName
|nominal
|path and file name

|LineNumber
|nominal
|integer

|FunctionName
|nominal
|bare function name

|FunctionSignature
|nominal
|function name and parameters

|RealTime
|ratio
|seconds

|===

== To Do Checklist

* [x] create GitHub project
* [x] create CMakeLists.txt file
* [x] set up project organization
* [x] start defining base macros and functions for Linux
* [ ] finish baseline profiling support
* [ ] add Linux performance counters
* [ ] finish baseline testing support
* [ ] add unit test suite
* [ ] add C++ wrapper
