# Figura 1 — Efeito do limite de banda sobre a evidência de defeito de rolamento.
#
# Sinal sintetizado. Compara a cadeia de referência (20 kHz) com a decimada a 1 kHz, com e
# sem filtro. Nenhuma das duas cadeias de 1 kHz é a do dispositivo, cujo filtro interno
# limita a banda a 184 Hz; por isso os rótulos não usam o nome do sensor.
#
# Arquétipo: grade quantitativa com painel-herói (a).
# Uso: Rscript experiments/figures/scripts/fig1_banda_sensor.R

# localiza o diretório do script sob Rscript ou source()
script_dir <- function() {
  a <- commandArgs(trailingOnly = FALSE)
  m <- grep("^--file=", a, value = TRUE)
  if (length(m)) return(dirname(normalizePath(sub("^--file=", "", m[1]))))
  "experiments/figures/scripts"
}
here <- script_dir()
source(file.path(here, "theme_kaelix.R"))

DATA <- file.path(here, "..", "data")
OUT  <- file.path(here, "..", "output")
dir.create(OUT, showWarnings = FALSE, recursive = TRUE)

rd <- function(f) readr::read_csv(file.path(DATA, f), show_col_types = FALSE)

NYQ    <- 500
FILTRO <- 184   # DLPF_CFG = 1 em src/sensors/vibration.cpp
ANOT   <- 2.2   # corpo das anotações, em mm (cerca de 6,3 pt)

# ---------------------------------------------------------------------------
# a — painel-herói: onde a energia do defeito fica no espectro
# ---------------------------------------------------------------------------
esp <- rd("fig1a_espectro.csv") |>
  mutate(condicao = factor(condicao, levels = c("sadio", "defeito")))

p_a <- ggplot(esp, aes(freq_hz, psd, colour = condicao)) +
  annotate("rect", xmin = 0, xmax = NYQ, ymin = 1e-11, ymax = 1e-1,
           fill = pal[["orange_wash"]], alpha = 0.13) +
  annotate("rect", xmin = 0, xmax = FILTRO, ymin = 1e-11, ymax = 1e-1,
           fill = pal[["orange_wash"]], alpha = 0.30) +
  geom_line(linewidth = 0.3) +
  annotate("segment", x = 3500, xend = 3500, y = 2.5e-3, yend = 6e-4,
           linewidth = 0.28, colour = pal[["neutral_dark"]],
           arrow = arrow(length = unit(1, "mm"), type = "closed")) +
  annotate("text", x = 3620, y = 9e-3, size = ANOT, hjust = 0.42,
           colour = pal[["neutral_dark"]], lineheight = 0.95,
           label = "ressonância excitada\npelos impactos (3,5 kHz)") +
  annotate("text", x = 590, y = 1.2e-8, hjust = 0, size = ANOT,
           colour = "#8a5a00", lineheight = 0.95,
           label = "até 500 Hz (Nyquist a 1 kHz)\nfaixa escura: 184 Hz, filtro do firmware") +
  scale_colour_manual(values = cond_cols) +
  scale_x_continuous(limits = c(0, 8000), breaks = seq(0, 8000, 2000),
                     labels = label_number(scale = 1e-3),
                     expand = expansion(mult = c(0.01, 0.02))) +
  scale_y_log10(limits = c(1e-11, 1e-1),
                breaks = 10^seq(-10, -2, 2), labels = label_log()) +
  labs(x = "Frequência (kHz)",
       y = expression("Densidade espectral ("*m^2*s^-4*Hz^-1*")")) +
  theme(legend.position = "inside", legend.position.inside = c(0.99, 0.98),
        legend.justification = c(1, 1),
        legend.key.height = unit(2.2, "mm"),
        axis.title.x = element_text(margin = margin(t = 1)))

# ---------------------------------------------------------------------------
# b — contraste na frequência característica, por método e banda
# ---------------------------------------------------------------------------
ctr <- rd("fig1b_contraste.csv") |>
  mutate(metodo_curto = factor(metodo_curto,
                               levels = c("envelope 20 kHz", "espectro 1 kHz")),
         tipo = ifelse(metodo_curto == "envelope 20 kHz", "correto", "incorreto"))

p_b <- ggplot(ctr, aes(metodo_curto, contraste, fill = tipo)) +
  geom_col(width = 0.5) +
  geom_text(aes(label = sprintf("%.0f vezes", contraste)),
            vjust = -0.5, size = ANOT) +
  scale_fill_manual(values = metodo_cols, guide = "none") +
  scale_x_discrete(labels = c("Envelope na ressonância,\n20 kHz",
                              "Espectro direto,\n1 kHz")) +
  scale_y_continuous(limits = c(0, 500), breaks = seq(0, 400, 200),
                     expand = expansion(mult = c(0, 0.05))) +
  labs(x = NULL, y = "Razão defeito/sadio")

# ---------------------------------------------------------------------------
# c — descritores de impacto: separação entre condições por cadeia
# ---------------------------------------------------------------------------
feat <- rd("fig1c_features.csv") |>
  mutate(razao = defeito / sadio,
         cadeia = factor(cadeia, levels = c("Referência 20 kHz",
                                            "MPU6050 c/ antialias",
                                            "MPU6050 s/ antialias"),
                         labels = c("20 kHz", "1 kHz com filtro", "1 kHz sem filtro")),
         feature = factor(feature, levels = c("curtose", "fator de crista",
                                              "pico", "pico-a-pico"),
                          labels = c("curtose", "fator de crista", "pico", "pico a pico")))

cadeia_cols <- c("20 kHz"           = pal[["signal_blue"]],
                 "1 kHz com filtro" = pal[["accent_red"]],
                 "1 kHz sem filtro" = pal[["accent_orange"]])

p_c <- ggplot(feat, aes(feature, razao, fill = cadeia)) +
  geom_hline(yintercept = 1, linewidth = 0.35, linetype = "22",
             colour = pal[["neutral_dark"]]) +
  geom_col(position = position_dodge(width = 0.75), width = 0.68) +
  scale_fill_manual(values = cadeia_cols) +
  scale_y_continuous(limits = c(0, 4.2), breaks = 0:3,
                     expand = expansion(mult = c(0, 0.02))) +
  labs(x = NULL, y = "Razão defeito/sadio") +
  theme(legend.position = "inside", legend.position.inside = c(0.99, 1),
        legend.justification = c(1, 1), legend.direction = "horizontal",
        legend.key.height = unit(2, "mm"), legend.key.width = unit(2.4, "mm"))

# ---------------------------------------------------------------------------
# d — rebatimento sem filtro
# ---------------------------------------------------------------------------
ali <- rd("fig1d_aliasing.csv")

p_d <- ggplot(ali, aes(f_real_hz, f_aparente_hz)) +
  annotate("rect", xmin = 2830, xmax = 5270, ymin = 0, ymax = NYQ,
           fill = pal[["orange_wash"]], alpha = 0.10) +
  geom_segment(aes(xend = f_real_hz, yend = 0),
               linewidth = 0.26, colour = pal[["neutral_light"]]) +
  geom_point(size = 0.95, colour = pal[["accent_red"]]) +
  geom_text(aes(label = sprintf("%.0f", f_aparente_hz)),
            vjust = -0.9, size = ANOT, colour = pal[["accent_red"]]) +
  scale_x_continuous(limits = c(2830, 5270), breaks = ali$f_real_hz,
                     labels = label_number(scale = 1e-3, accuracy = 0.1, decimal.mark = ",")) +
  scale_y_continuous(limits = c(0, 640), breaks = seq(0, 500, 250)) +
  annotate("text", x = 2880, y = 575, hjust = 0, size = ANOT,
           colour = "#8a5a00", label = "até 500 Hz") +
  labs(x = "Frequência real da ressonância (kHz)",
       y = "Frequência\naparente (Hz)")

# ---------------------------------------------------------------------------
# montagem: herói à esquerda (a), evidência de apoio à direita (b, c),
# consequência embaixo (d)
# ---------------------------------------------------------------------------
fig1 <- (p_a | (p_b / p_c + plot_layout(heights = c(1, 1.15)))) /
        p_d +
  plot_layout(heights = c(2.7, 1), widths = c(1.3, 1)) +
  plot_annotation(tag_levels = "a") &
  tag_theme

save_pub(fig1, file.path(OUT, "fig1_banda_sensor"),
         width_mm = 183, height_mm = 110)
