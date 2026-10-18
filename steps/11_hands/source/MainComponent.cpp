#include "MainComponent.h"

#include <algorithm>

namespace hands {

namespace {

constexpr int k_panel_width = 360;
constexpr float k_latent_display_range = 3.5F;

const juce::Colour k_background{0xff1c1d21};
const juce::Colour k_panel{0xff2a2b30};
const juce::Colour k_text{0xfffafafa};
const juce::Colour k_dim_text{0xff8a8a90};
const juce::Colour k_accent{0xffbf85fc};

juce::Colour role_colour(Role role) {
    return role == Role::Lead ? k_accent : juce::Colour(0xff7fa592);
}

// The tracker may keep the frame beyond the call: keep_alive holds the image's refcount.
ht::Frame to_frame(const juce::Image& image) {
    auto retained = std::make_shared<juce::Image>(image);
    const juce::Image::BitmapData data(*retained, juce::Image::BitmapData::readOnly);
    ht::Frame frame;
    frame.pixels = data.data;
    frame.width = data.width;
    frame.height = data.height;
    frame.stride = data.lineStride;
    frame.pixel_stride = data.pixelStride;
    frame.format = ht::PixelFormat::BGRA;  // JUCE ARGB on little-endian
    frame.keep_alive = std::move(retained);
    return frame;
}

void setup_slider(juce::Slider& slider, juce::Label& label, const juce::String& name,
                  double min, double max, double value, const juce::String& suffix) {
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 20);
    slider.setRange(min, max, 0.01);
    slider.setValue(value, juce::dontSendNotification);
    slider.setTextValueSuffix(suffix);
    label.setText(name, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, k_dim_text);
}

}  // namespace

MainComponent::MainComponent() {
    m_tracker = ht::createHandTracker(ht::Engine::Streamed);
    auto options = m_tracker->getOptions();
    options.num_hands = 2;
    m_tracker->setOptions(options);

    try {
        m_decoder = std::make_unique<LatentDecoder>(WORKSHOP_STEP_DIR);
    } catch (const std::exception& e) {
        m_decoder_error = juce::String("The decoder failed to load: ") + e.what();
    }

    if (m_decoder != nullptr) {
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        setup.sampleRate = k_model_sample_rate;  // anything else is resampled
        setup.bufferSize = 512;
        const auto error = m_device_manager.initialise(0, 2, nullptr, true, {}, &setup);
        if (error.isNotEmpty())
            m_decoder_error = "Audio device: " + error;
        m_player.setSource(m_decoder.get());
        m_device_manager.addAudioCallback(&m_player);
    }

    setup_slider(m_volume, m_volume_label, "Volume", 0.0, 1.5, 0.8, "");
    setup_slider(m_smoothing, m_smoothing_label, "Smoothing", 0.0, 400.0, 80.0, " ms");
    setup_slider(m_spread, m_spread_label, "Spread", 0.25, 1.5, 1.0, "x");
    m_smoothing.setNumDecimalPlacesToDisplay(0);
    m_volume.onValueChange = [this] {
        if (m_decoder != nullptr)
            m_decoder->setVolume(static_cast<float>(m_volume.getValue()));
    };
    m_volume.onValueChange();
    m_silence_without_hands.setToggleState(true, juce::dontSendNotification);
    m_audio_settings.onClick = [this] { showAudioSettings(); };

    for (juce::Component* child : std::initializer_list<juce::Component*>{
             &m_volume, &m_smoothing, &m_spread, &m_volume_label, &m_smoothing_label, &m_spread_label,
             &m_swap_hands, &m_silence_without_hands, &m_audio_settings})
        addAndMakeVisible(child);

    m_camera = std::make_unique<CameraInput>(
        [this](const juce::Image& image) { onCameraFrame(image); },
        [safe = juce::Component::SafePointer<MainComponent>(this)](const juce::String& error) {
            if (safe != nullptr) {
                safe->m_camera_error = error;
                safe->m_pad = true;  // No camera: the mouse plays
                safe->repaint();
            }
        });
    m_camera->start();

    setSize(1280, 760);
    m_last_tick = juce::Time::getMillisecondCounterHiRes() * 1e-3;
    startTimerHz(60);
}

MainComponent::~MainComponent() {
    stopTimer();
    m_camera.reset();
    m_device_manager.removeAudioCallback(&m_player);
    m_player.setSource(nullptr);
}

void MainComponent::onCameraFrame(const juce::Image& image) {
    m_tracker->processFrame(to_frame(image));
    const juce::SpinLock::ScopedLockType lock(m_frame_lock);
    m_pending_frame = image;
}

void MainComponent::timerCallback() {
    const double now = juce::Time::getMillisecondCounterHiRes() * 1e-3;
    const double dt = std::clamp(now - m_last_tick, 0.0, 0.1);
    m_last_tick = now;

    {
        const juce::SpinLock::ScopedLockType lock(m_frame_lock);
        if (m_pending_frame.isValid())
            m_frame = std::exchange(m_pending_frame, {});
    }
    m_result = m_tracker->getLatestResult();

    HandControls::Settings settings;
    settings.smoothing_ms = static_cast<float>(m_smoothing.getValue());
    settings.spread = static_cast<float>(m_spread.getValue());
    settings.swap_hands = m_swap_hands.getToggleState();

    if (m_pad) {
        m_controls.updateFeatures(m_pad_features, std::nullopt, dt, settings);
    } else {
        const float aspect = m_frame.isValid() ? static_cast<float>(m_frame.getWidth()) / static_cast<float>(m_frame.getHeight())
                                               : 16.0F / 9.0F;
        m_controls.update(m_result, aspect, dt, settings);
    }

    if (m_decoder != nullptr) {
        m_decoder->setLatents(m_controls.latents());
        m_decoder->setSounding(!m_silence_without_hands.getToggleState() || m_controls.anyHandPresent());
    }
    repaint();
}

// The pad: position is left-right and height, the button is the open hand.
void MainComponent::padFrom(const juce::MouseEvent& event) {
    const auto area = videoBounds();
    if (!area.contains(event.position)) {
        m_pad_features.reset();
        return;
    }
    HandFeatures features;
    features.x = (event.position.x - area.getX()) / area.getWidth();
    features.y = 1.0F - (event.position.y - area.getY()) / area.getHeight();
    features.openness = event.mods.isLeftButtonDown() ? 1.0F : 0.3F;
    features.pinch = event.mods.isRightButtonDown() ? 1.0F : 0.0F;
    m_pad_features = features;
}

void MainComponent::mouseDown(const juce::MouseEvent& event) {
    if (m_pad) padFrom(event);
}

void MainComponent::mouseDrag(const juce::MouseEvent& event) {
    if (m_pad) padFrom(event);
}

void MainComponent::mouseUp(const juce::MouseEvent&) {
    if (m_pad) m_pad_features.reset();
}

void MainComponent::showAudioSettings() {
    auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(m_device_manager, 0, 0, 2, 2, false, false, true, false);
    selector->setSize(500, 360);
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(selector.release());
    options.dialogTitle = "Audio settings";
    options.dialogBackgroundColour = k_panel;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}

juce::Rectangle<float> MainComponent::videoBounds() const {
    const auto area = getLocalBounds().withTrimmedRight(k_panel_width).reduced(16).toFloat();
    const float aspect = m_frame.isValid() ? static_cast<float>(m_frame.getWidth()) / static_cast<float>(m_frame.getHeight())
                                           : 16.0F / 9.0F;
    const float width = std::min(area.getWidth(), area.getHeight() * aspect);
    return juce::Rectangle<float>(width, width / aspect).withCentre(area.getCentre());
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(k_background);
    const auto video = videoBounds();
    paintVideo(g, video);
    paintHands(g, video);

    auto panel = getLocalBounds().removeFromRight(k_panel_width);
    g.setColour(k_panel);
    g.fillRect(panel);
    paintLatents(g, panel.reduced(18, 16));
}

void MainComponent::paintVideo(juce::Graphics& g, juce::Rectangle<float> area) const {
    g.setColour(juce::Colours::black);
    g.fillRect(area);

    if (m_frame.isValid()) {
        // Mirrored, like a mirror: moving right moves right on screen.
        const float scale = area.getWidth() / static_cast<float>(m_frame.getWidth());
        g.drawImageTransformed(m_frame, juce::AffineTransform::scale(-scale, scale).translated(area.getRight(), area.getY()));
    } else if (m_pad) {
        g.setColour(k_dim_text);
        g.setFont(16.0F);
        g.drawText(m_camera_error + "  The pad plays instead: drag here, hold the button to open the hand.",
                   area, juce::Justification::centred);
        if (m_pad_features.has_value()) {
            const juce::Point<float> at(area.getX() + m_pad_features->x * area.getWidth(),
                                        area.getBottom() - m_pad_features->y * area.getHeight());
            g.setColour(role_colour(Role::Lead));
            g.fillEllipse(juce::Rectangle<float>(18.0F, 18.0F).withCentre(at));
        }
    } else {
        g.setColour(k_dim_text);
        g.setFont(16.0F);
        g.drawText("Waiting for the camera...", area, juce::Justification::centred);
    }

    // Status line.
    juce::String status;
    status << "camera " << juce::roundToInt(m_result.camera_fps) << " fps   tracking "
           << juce::roundToInt(m_result.tracking_fps) << " fps (" << ht::backend_name(m_tracker->getBackend()) << ")";
    if (m_decoder != nullptr)
        status << "   decoder latency " << juce::roundToInt(1000.0 * m_decoder->getLatencySamples() / k_model_sample_rate) << " ms";
    g.setFont(13.0F);
    auto bar = area.removeFromTop(26.0F);
    g.setColour(juce::Colours::black.withAlpha(0.55F));
    g.fillRect(bar);
    g.setColour(k_text);
    g.drawText(status, bar.reduced(10.0F, 0.0F), juce::Justification::centredLeft);

    juce::String banner = m_decoder_error;
    if (banner.isEmpty() && mappings_are_empty())
        banner = "TODO 1: every range in the mapping table is empty, the hand changes nothing (HandControls.cpp)";
    if (banner.isNotEmpty()) {
        const auto strip = area.removeFromBottom(30.0F);
        g.setColour(juce::Colour(0xfffe938c).withAlpha(0.85F));
        g.fillRect(strip);
        g.setColour(k_background);
        g.drawText(banner, strip.reduced(10.0F, 0.0F), juce::Justification::centredLeft);
    }
}

void MainComponent::paintHands(juce::Graphics& g, juce::Rectangle<float> video) const {
    if (m_pad) return;
    const auto to_screen = [video](ht::Vec2 p) {
        return juce::Point<float>(video.getRight() - p.x * video.getWidth(), video.getY() + p.y * video.getHeight());
    };

    for (size_t i = 0; i < m_result.num_hands; ++i) {
        const auto& hand = m_result.hands[i];
        juce::Colour colour = k_dim_text;
        const char* label = "";
        for (const Role role : {Role::Lead, Role::Texture}) {
            if (m_controls.slotForRole(role) == hand.slot) {
                colour = role_colour(role);
                label = role == Role::Lead ? "LEAD" : "TEXTURE";
            }
        }

        g.setColour(colour.withAlpha(0.9F));
        for (const auto& edge : ht::k_hand_connections)
            g.drawLine(juce::Line<float>(to_screen(hand.landmarks[static_cast<size_t>(edge[0])]),
                                         to_screen(hand.landmarks[static_cast<size_t>(edge[1])])),
                       2.5F);
        for (const auto& landmark : hand.landmarks)
            g.fillEllipse(juce::Rectangle<float>(7.0F, 7.0F).withCentre(to_screen(landmark)));

        juce::String text(label);
        if (hand.gesture > 0)
            text << "  " << ht::k_gesture_labels[static_cast<size_t>(hand.gesture)];
        const auto wrist = to_screen(hand.landmarks[0]);
        g.setFont(juce::FontOptions(15.0F, juce::Font::bold));
        g.drawText(text, juce::Rectangle<float>(220.0F, 22.0F).withCentre(wrist.translated(0.0F, 26.0F)), juce::Justification::centred);
    }
}

void MainComponent::paintLatents(juce::Graphics& g, juce::Rectangle<int> area) const {
    g.setColour(k_text);
    g.setFont(juce::FontOptions(18.0F, juce::Font::bold));
    g.drawText("Latents", area.removeFromTop(24), juce::Justification::centredLeft);
    g.setColour(k_dim_text);
    g.setFont(12.0F);
    g.drawText("magnetic resonator piano, eight of them", area.removeFromTop(18), juce::Justification::centredLeft);
    area.removeFromTop(10);

    const auto& latents = m_controls.latents();
    const auto& targets = m_controls.targets();
    for (size_t i = 0; i < k_num_latents; ++i) {
        const auto& mapping = mappings()[i];
        auto row = area.removeFromTop(42);
        const auto colour = role_colour(mapping.role);
        const bool active = m_controls.rolePresent(mapping.role);

        auto caption = row.removeFromTop(18);
        g.setFont(juce::FontOptions(13.0F, juce::Font::bold));
        g.setColour(k_text);
        g.drawText("z" + juce::String(static_cast<int>(i)) + "  " + mapping.sound, caption, juce::Justification::centredLeft);
        g.setFont(12.0F);
        g.setColour(active ? colour : k_dim_text);
        g.drawText(juce::String(role_name(mapping.role)) + " " + feature_name(mapping.feature) + "   "
                       + juce::String(latents[i], 2),
                   caption, juce::Justification::centredRight);

        const auto track = row.removeFromTop(14).toFloat();
        g.setColour(juce::Colours::white.withAlpha(0.06F));
        g.fillRoundedRectangle(track, 3.0F);
        const auto x_of = [track](float value) {
            const float t = (std::clamp(value, -k_latent_display_range, k_latent_display_range) + k_latent_display_range) / (2.0F * k_latent_display_range);
            return track.getX() + t * track.getWidth();
        };
        // Mapped range, value bar from 0, target tick.
        g.setColour(colour.withAlpha(0.12F));
        g.fillRect(juce::Rectangle<float>::leftTopRightBottom(x_of(std::min(mapping.at_0, mapping.at_1)), track.getY(),
                                                               x_of(std::max(mapping.at_0, mapping.at_1)), track.getBottom()));
        g.setColour(colour.withAlpha(active ? 0.85F : 0.35F));
        const float zero = x_of(0.0F);
        const float value = x_of(latents[i]);
        g.fillRect(juce::Rectangle<float>::leftTopRightBottom(std::min(zero, value), track.getY() + 2.0F,
                                                               std::max(zero, value), track.getBottom() - 2.0F));
        g.setColour(k_text.withAlpha(0.7F));
        g.fillRect(juce::Rectangle<float>(1.0F, track.getHeight()).withCentre({zero, track.getCentreY()}));
        g.setColour(k_text);
        g.fillRect(juce::Rectangle<float>(2.0F, track.getHeight() + 4.0F).withCentre({x_of(targets[i]), track.getCentreY()}));
    }

    // Output level.
    area.removeFromTop(6);
    auto meter = area.removeFromTop(10).toFloat();
    g.setColour(juce::Colours::white.withAlpha(0.06F));
    g.fillRoundedRectangle(meter, 3.0F);
    if (m_decoder != nullptr) {
        const float db = juce::Decibels::gainToDecibels(m_decoder->getOutputLevel(), -60.0F);
        g.setColour(juce::Colour(0xff7fa592));
        g.fillRoundedRectangle(meter.withWidth(meter.getWidth() * (db + 60.0F) / 60.0F), 3.0F);
    }
}

void MainComponent::resized() {
    auto area = getLocalBounds().removeFromRight(k_panel_width).reduced(18, 16);
    area.removeFromTop(24 + 18 + 10 + 42 * static_cast<int>(k_num_latents) + 6 + 10 + 14);

    const auto slider_row = [&area](juce::Label& label, juce::Slider& slider) {
        auto row = area.removeFromTop(28);
        label.setBounds(row.removeFromLeft(80));
        slider.setBounds(row);
    };
    slider_row(m_volume_label, m_volume);
    slider_row(m_smoothing_label, m_smoothing);
    slider_row(m_spread_label, m_spread);
    area.removeFromTop(4);
    m_swap_hands.setBounds(area.removeFromTop(24));
    m_silence_without_hands.setBounds(area.removeFromTop(24));
    area.removeFromTop(6);
    m_audio_settings.setBounds(area.removeFromTop(28));
}

}  // namespace hands
