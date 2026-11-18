#pragma once
// Bodytone SMB1 V3: experimental, conservative FTMS simulation-grade ERG controller.
// This header is intentionally independent of Qt so its math can be unit tested.
// WARNING: Grade commands are NOT guaranteed to correspond to physical levels.
// Never treat this as a safety-rated control system.
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace bodytone {
class SafeErgController {
public:
    struct Command {
        bool send;
        double gradePercent;
        const char* reason;
        Command(bool s = false, double g = 0.0, const char* r = "none")
            : send(s), gradePercent(g), reason(r) {}
    };
    static constexpr double kMaxGrade = 5.0;
    static constexpr double kMaxUpStep = 0.25;
    static constexpr double kMaxDownStep = 1.0;
    static constexpr std::int64_t kMinCommandIntervalMs = 4000;
    static constexpr std::int64_t kCommandTimeoutMs = 45000;

    void setTarget(int watts, std::int64_t nowMs) {
        if (watts <= 0 || watts > 1000) { cancel(); return; }
        targetWatts_ = watts;
        lastTargetAt_ = nowMs;
        if (!active_) {
            active_ = true;
            neutralPending_ = true;
            gradePercent_ = 0.0;
            lastCommandAt_ = -1;
            validSamples_ = 0;
            lastCadence_ = 0.0;
        }
    }
    void cancel() {
        active_ = false;
        neutralPending_ = false;
        targetWatts_ = 0;
        validSamples_ = 0;
        lastCadence_ = 0.0;
        lastTargetAt_ = -1;
    }
    bool active() const { return active_; }
    int targetWatts() const { return targetWatts_; }
    double gradePercent() const { return gradePercent_; }

    Command tick(double cadence, double measuredWatts, bool fresh, std::int64_t nowMs) {
        if (!active_) return {};
        if (lastTargetAt_ >= 0 &&
            (nowMs < lastTargetAt_ || nowMs - lastTargetAt_ > kCommandTimeoutMs)) {
            const bool loaded = gradePercent_ > 0.0;
            cancel();
            return {loaded, 0.0, "target-command-timeout-neutral"};
        }
        if (neutralPending_) {
            neutralPending_ = false;
            gradePercent_ = 0.0;
            lastCommandAt_ = nowMs;
            return {true, gradePercent_, "enter-erg-neutral"};
        }
        if (lastCommandAt_ >= 0 && nowMs - lastCommandAt_ < kMinCommandIntervalMs)
            return {};
        const bool sensorInvalid = !fresh || !std::isfinite(cadence) ||
                !std::isfinite(measuredWatts) || cadence <= 0.0 || cadence > 180.0 ||
                measuredWatts < 0.0 || measuredWatts > 1500.0;
        if (sensorInvalid || cadence < 45.0) {
            validSamples_ = 0;
            lastCadence_ = 0.0;
            if (gradePercent_ > 0.0)
                return sendClamped(0.0, nowMs, "emergency-neutral-invalid-sensor-or-cadence");
            return {};
        }
        if (cadence < 55.0 || measuredWatts < 10.0) {
            validSamples_ = 0;
            lastCadence_ = 0.0;
            if (gradePercent_ > 0.0)
                return sendClamped(gradePercent_ - kMaxDownStep, nowMs, "low-cadence-or-zero-watts");
            return {};
        }
        if (gradePercent_ > 0.0 && measuredWatts > targetWatts_ * 1.75 &&
                measuredWatts - targetWatts_ > 150.0) {
            validSamples_ = 0;
            return sendClamped(0.0, nowMs, "emergency-neutral-extreme-power");
        }
        ++validSamples_;
        if (validSamples_ > 3) validSamples_ = 3;
        const bool cadenceDropping = lastCadence_ > 0 && cadence < lastCadence_ - 7.0;
        lastCadence_ = cadence;
        if (validSamples_ < 2) return {};
        const double error = static_cast<double>(targetWatts_) - measuredWatts;
        const double tolerance = std::max(12.0, static_cast<double>(targetWatts_) * 0.04);
        if (std::fabs(error) <= tolerance) return {};
        if (error > 0.0) {
            if (cadence < 65.0 || cadenceDropping) return {};
            const double rawStep = std::max(0.2, error / 200.0);
            const double step = rawStep < kMaxUpStep ? rawStep : kMaxUpStep;
            if (gradePercent_ >= kMaxGrade) return {};
            return sendClamped(gradePercent_ + step, nowMs, "measured-power-below-target");
        }
        const double step = (-error > std::max(50.0, targetWatts_ * 0.25))
                                ? kMaxDownStep : 0.5;
        if (gradePercent_ <= 0.0) return {};
        return sendClamped(gradePercent_ - step, nowMs, "measured-power-above-target");
    }
private:
    Command sendClamped(double proposedGrade, std::int64_t nowMs, const char* reason) {
        const double next = proposedGrade < 0.0 ? 0.0 : (proposedGrade > kMaxGrade ? kMaxGrade : proposedGrade);
        if (std::fabs(next - gradePercent_) < 0.0001) return {};
        gradePercent_ = next;
        lastCommandAt_ = nowMs;
        return {true, gradePercent_, reason};
    }
    bool active_ = false;
    bool neutralPending_ = false;
    int targetWatts_ = 0;
    double gradePercent_ = 0.0;
    double lastCadence_ = 0.0;
    int validSamples_ = 0;
    std::int64_t lastCommandAt_ = -1;
    std::int64_t lastTargetAt_ = -1;
};
} // namespace bodytone
