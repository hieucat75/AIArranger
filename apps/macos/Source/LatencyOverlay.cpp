#include "LatencyOverlay.h"

namespace ai_arranger::app {

void LatencyOverlayPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff15151a));
    auto r = getLocalBounds().reduced(8);

    g.setColour(juce::Colour(0xffededf2));
    auto header = r.removeFromTop(20);
    g.drawText(juce::String::formatted("latency avg %.2f  max %.2f  jitter %.2f ms",
               src_.average(), src_.maximum(), src_.jitter()),
               header, juce::Justification::centredLeft);

    int h[performance::LatencyOverlay::kBuckets];
    src_.histogram(2.0 /* ms/bucket */, h);
    int maxc = 1;
    for (int i = 0; i < performance::LatencyOverlay::kBuckets; ++i) maxc = juce::jmax(maxc, h[i]);

    const int n = performance::LatencyOverlay::kBuckets;
    const float bw = (float)r.getWidth() / n;
    for (int i = 0; i < n; ++i) {
        const float frac = (float)h[i] / maxc;
        const float bh = frac * r.getHeight();
        juce::Rectangle<float> bar(r.getX() + i * bw, r.getBottom() - bh, bw - 2, bh);
        g.setColour(juce::Colour(0xff18d2e6));
        g.fillRect(bar);
    }
}

} // namespace ai_arranger::app
