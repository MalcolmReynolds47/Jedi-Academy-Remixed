// Lifetime limits and clipboard values for Sandbox pickups. GPL-2.0-only.
#pragma once
namespace jar { namespace pickupLifetime {
constexpr int defaultMs=30000,maxMs=600000;
constexpr int keepDeadline=-3;
constexpr int giveDeadline=-2;
} }
