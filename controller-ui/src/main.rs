#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use eframe::egui::{self, Color32, FontId, RichText, Vec2};

const ACCENT: Color32 = Color32::from_rgb(137, 211, 165);
const MUTED: Color32 = Color32::from_rgb(143, 148, 156);

#[derive(Clone, Copy, Default, PartialEq, Eq)]
enum Filter {
    #[default]
    None,
    Grayscale,
    Ascii,
}

impl Filter {
    const ALL: [Self; 3] = [Self::None, Self::Grayscale, Self::Ascii];

    fn label(self) -> &'static str {
        match self {
            Self::None => "None",
            Self::Grayscale => "Grayscale",
            Self::Ascii => "ASCII",
        }
    }

    fn description(self) -> &'static str {
        match self {
            Self::None => "Keep the original colors.",
            Self::Grayscale => "Convert the image to black and white.",
            Self::Ascii => "Render the image with ASCII characters.",
        }
    }
}

// UI state only. No camera APIs, subprocesses or shared memory are used.
#[derive(Default)]
struct CameraApp {
    enabled: bool,
    filter: Filter,
}

impl CameraApp {
    fn new(cc: &eframe::CreationContext<'_>) -> Self {
        let mut style = (*cc.egui_ctx.style()).clone();
        style.visuals = egui::Visuals::dark();
        style.visuals.panel_fill = Color32::from_rgb(22, 24, 28);
        style.visuals.selection.bg_fill = Color32::from_rgb(48, 77, 61);
        style.visuals.selection.stroke.color = ACCENT;
        style.spacing.item_spacing = Vec2::new(12.0, 14.0);
        style.spacing.button_padding = Vec2::new(18.0, 9.0);
        style
            .text_styles
            .insert(egui::TextStyle::Body, FontId::monospace(14.0));
        style
            .text_styles
            .insert(egui::TextStyle::Button, FontId::monospace(14.0));
        cc.egui_ctx.set_style(style);
        Self::default()
    }
}

impl eframe::App for CameraApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        egui::CentralPanel::default()
            .frame(
                egui::Frame::new()
                    .fill(Color32::from_rgb(22, 24, 28))
                    .inner_margin(26.0),
            )
            .show(ctx, |ui| {
                ui.horizontal(|ui| {
                    ui.label(RichText::new("ASCII CAM").monospace().size(23.0).strong());
                    ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                        ui.label(
                            RichText::new("UI PREVIEW")
                                .monospace()
                                .size(11.0)
                                .color(MUTED),
                        );
                    });
                });
                ui.label(RichText::new("Virtual camera controller").color(MUTED));
                ui.add_space(10.0);
                ui.separator();
                ui.add_space(7.0);

                ui.horizontal(|ui| {
                    ui.label("Camera");
                    ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                        // Separate ON/OFF choices also expose selected state to accessibility tools.
                        ui.selectable_value(&mut self.enabled, false, "OFF");
                        ui.selectable_value(&mut self.enabled, true, "ON");
                    });
                });
                ui.horizontal(|ui| {
                    let color = if self.enabled { ACCENT } else { MUTED };
                    ui.colored_label(
                        color,
                        if self.enabled {
                            "ON selected"
                        } else {
                            "OFF selected"
                        },
                    );
                    ui.label(RichText::new("/ preview only").color(MUTED).size(12.0));
                });

                ui.add_space(6.0);
                ui.separator();
                ui.add_space(7.0);
                ui.horizontal(|ui| {
                    ui.label("Filter");
                    ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                        egui::ComboBox::from_id_salt("filter_choice")
                            .width(180.0)
                            .selected_text(self.filter.label())
                            .show_ui(ui, |ui| {
                                for filter in Filter::ALL {
                                    ui.selectable_value(&mut self.filter, filter, filter.label());
                                }
                            });
                    });
                });
                ui.label(
                    RichText::new(self.filter.description())
                        .color(MUTED)
                        .size(12.0),
                );
                if !self.enabled {
                    ui.label(
                        RichText::new("Choose a filter before switching ON.")
                            .color(MUTED)
                            .size(12.0),
                    );
                }

                ui.add_space(10.0);
                ui.separator();
                ui.label(
                    RichText::new("Backend: not connected")
                        .monospace()
                        .color(MUTED)
                        .size(12.0),
                );
                ui.label(
                    RichText::new("Controls change this preview only.")
                        .color(MUTED)
                        .size(12.0),
                );
            });
    }
}

fn main() -> eframe::Result {
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([460.0, 410.0])
            .with_min_inner_size([420.0, 400.0]),
        ..Default::default()
    };
    eframe::run_native(
        "ASCII Cam",
        options,
        Box::new(|cc| Ok(Box::new(CameraApp::new(cc)))),
    )
}
