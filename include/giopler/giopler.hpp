// Copyright (c) 2023 Giopler
// Licensed under the Creative Commons Attribution-NoDerivatives 4.0 International license.
// SPDX-License-Identifier: CC-BY-ND-4.0
// https://creativecommons.org/licenses/by-nd/4.0/

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
