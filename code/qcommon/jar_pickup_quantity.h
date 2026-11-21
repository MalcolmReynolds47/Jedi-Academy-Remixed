// Single-player Sandbox pickup quantity limits. GPL-2.0-only.
#pragma once
namespace jar { namespace pickupQuantity {
constexpr int maximum=1000;
inline bool Valid(int value){return value>=0&&value<=maximum;}
} }
