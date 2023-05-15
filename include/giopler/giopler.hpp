// Copyright (c) 2023 Giopler
// Creative Commons Attribution No Derivatives 4.0 International license
// https://creativecommons.org/licenses/by-nd/4.0
// SPDX-License-Identifier: CC-BY-ND-4.0
//
// Share         — copy and redistribute the material in any medium or format for any purpose, even commercially.
// NoDerivatives — If you remix, transform, or build upon the material, you may not distribute the modified material.
// Attribution   — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
//                 You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.

#pragma once
#ifndef GIOPLER_GIOPLER_HPP
#define GIOPLER_GIOPLER_HPP

#if __cplusplus < 202002L
#error Support for C++20 or newer is required to use this library.
#endif

// -----------------------------------------------------------------------------
#include "giopler/config.hpp"
#include "giopler/platform.hpp"
#include "giopler/utility.hpp"
#include "giopler/record.hpp"
#include "giopler/sink.hpp"
#include "giopler/exit.hpp"

#include "giopler/contract.hpp"
#include "giopler/trace.hpp"
#include "giopler/log.hpp"

#include "giopler/counter.hpp"
#include "giopler/profile.hpp"

// -----------------------------------------------------------------------------
#endif // defined GIOPLER_GIOPLER_HPP
