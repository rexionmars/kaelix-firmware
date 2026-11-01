# Figura 2 — Efeito de quatro escolhas metodológicas sobre os resultados.
#
# Sinal sintetizado. Integração (a, b), partição (c), parâmetro de contaminação (d) e banda
# com área parcial (e). O painel c usa um Random Forest supervisionado, e o d mede o falso
# alarme nas janelas do ajuste; a legenda no texto informa as duas condições.
#
# Arquétipo: grade quantitativa, blocos de evidência independentes.
# Uso: Rscript experiments/figures/scripts/fig2_metodologia.R

script_dir <- function() {
  a <- commandArgs(trailingOnly = FALSE)
  m <- grep("^--file=", a, value = TRUE)
  if (length(m)) return(dirname(normalizePath(sub("^--file=", "", m[1]))))
  "experiments/figures/scripts"
}
here <- script_dir()
source(file.path(here, "theme_kaelix.R"))

data_dir <- file.path(here, "..", "data")
out_dir  <- file.path(here, "..", "output")
dir.create(out_dir, showWarnings = FALSE, recursive = TRUE)

rd <- function(f) readr::read_csv(file.path(data_dir, f), show_col_types = FALSE)

ANOT <- 2.2   # corpo das anotações, em mm (cerca de 6,3 pt)
virgula <- function(x) gsub(".", ",", x, fixed = TRUE)
num2 <- function(x) virgula(sprintf("%.2f", x))
num3 <- function(x) virgula(sprintf("%.3f", x))
pct1 <- function(x) virgula(sprintf("%.1f %%", 100 * x))
eixo_decimal <- function(x) virgula(sprintf("%.1f", x))

v_exato <- rd("fig2_referencia.csv")$valor_exato_mm_s[1]

# ---------------------------------------------------------------------------
# a — velocidade eficaz de cada variante contra o valor analítico
# ---------------------------------------------------------------------------
nomes_metodo <- c("No tempo, sem filtro",
                  "No tempo, com filtro",
                  "Na frequência",
                  "Na frequência, com viés",
                  "No tempo, sem filtro,\ncom viés")

intg <- rd("fig2a_integracao.csv") |>
  arrange(ordem) |>
  mutate(rotulo = nomes_metodo,
         rotulo = factor(rotulo, levels = rev(nomes_metodo)),
         tipo = ifelse(correto, "correto", "incorreto"))

p_a <- ggplot(intg, aes(rms_mm_s, rotulo, fill = tipo)) +
  geom_vline(xintercept = v_exato, linewidth = 0.4,
             colour = pal[["neutral_dark"]], linetype = "22") +
  geom_col(width = 0.6) +
  geom_text(aes(label = ifelse(abs(erro_pct) < 0.01, "erro abaixo de 0,01 %",
                               sprintf("+%.0f %%", erro_pct))),
            hjust = -0.1, size = ANOT,
            colour = ifelse(intg$correto, pal[["signal_blue"]], pal[["accent_red"]])) +
  annotate("text", x = v_exato, y = 5.72, size = ANOT, hjust = -0.05,
           colour = pal[["neutral_dark"]],
           label = sprintf("valor analítico: %s mm/s", num2(v_exato))) +
  scale_fill_manual(values = metodo_cols, guide = "none") +
  scale_x_continuous(limits = c(0, 72), breaks = seq(0, 60, 20),
                     expand = expansion(mult = c(0, 0.02))) +
  coord_cartesian(clip = "off") +
  labs(x = "Velocidade eficaz (mm/s)", y = NULL) +
  theme(axis.text.y = element_text(lineheight = 0.85),
        plot.margin = margin(8, 8, 2, 2))

# ---------------------------------------------------------------------------
# b — deriva sob viés
# ---------------------------------------------------------------------------
der <- rd("fig2b_deriva.csv") |>
  mutate(metodo = factor(metodo, levels = c("no tempo, sem passa-alta", "na frequência"),
                         labels = c("no tempo, sem filtro", "na frequência")))

p_b <- ggplot(der, aes(tempo_s, velocidade_mm_s, colour = metodo)) +
  geom_hline(yintercept = v_exato, linewidth = 0.35,
             colour = pal[["neutral_dark"]], linetype = "22") +
  geom_line(linewidth = 0.22) +
  scale_colour_manual(values = c("no tempo, sem filtro" = pal[["accent_red"]],
                                 "na frequência" = pal[["signal_blue"]])) +
  scale_x_continuous(breaks = seq(0, 4, 1), expand = expansion(mult = 0.01)) +
  scale_y_continuous(limits = c(-25, 105), breaks = seq(0, 100, 50)) +
  labs(x = "Tempo (s)", y = "Velocidade (mm/s)") +
  theme(legend.position = "inside", legend.position.inside = c(0.02, 0.97),
        legend.justification = c(0, 1), legend.key.height = unit(2.2, "mm"))

# ---------------------------------------------------------------------------
# c — partição
# ---------------------------------------------------------------------------
spl   <- rd("fig2c_split.csv")
folds <- rd("fig2c_folds.csv")
base  <- rd("fig2c_baseline.csv")$baseline[1]

spl <- spl |>
  arrange(ordem) |>
  mutate(rotulo = factor(c("Aleatória\npor janela", "Por ensaio"),
                         levels = c("Aleatória\npor janela", "Por ensaio")),
         tipo = ifelse(tipo == "correto", "correto", "incorreto"))

p_c <- ggplot(spl, aes(rotulo, media, fill = tipo)) +
  geom_hline(yintercept = base, linewidth = 0.35,
             colour = pal[["neutral_mid"]], linetype = "22") +
  geom_col(width = 0.5) +
  geom_errorbar(aes(ymin = media - desvio, ymax = pmin(media + desvio, 1)),
                width = 0.13, linewidth = 0.3) +
  geom_point(data = folds, aes(x = 2, y = acuracia), inherit.aes = FALSE,
             position = position_jitter(width = 0.1, height = 0, seed = 1),
             size = 0.75, colour = pal[["neutral_dark"]], alpha = 0.85) +
  geom_text(aes(label = pct1(media), y = media + desvio + 0.05), size = ANOT) +
  annotate("text", x = 1.5, y = base + 0.04, size = ANOT,
           colour = pal[["neutral_mid"]], label = "acaso") +
  scale_fill_manual(values = metodo_cols, guide = "none") +
  scale_y_continuous(limits = c(0, 1.14), breaks = seq(0, 1, 0.25),
                     labels = label_percent(accuracy = 1, suffix = " %"),
                     expand = expansion(mult = c(0, 0.02))) +
  labs(x = "Partição", y = "Acurácia") +
  theme(axis.text.x = element_text(lineheight = 0.85),
        axis.title.y = element_text(margin = margin(r = 0)),
        plot.margin = margin(2, 2, 2, 0))

# ---------------------------------------------------------------------------
# d — parâmetro de contaminação e falso alarme nas janelas do ajuste
# ---------------------------------------------------------------------------
cont <- rd("fig2d_contamination.csv") |>
  mutate(contamination = factor(contamination,
                                levels = c("0.5", "0.2", "0.1", "0.01", "auto"),
                                labels = c("0,5", "0,2", "0,1", "0,01", "padrão")),
         tipo = ifelse(is_default, "incorreto", "correto"))

p_d <- ggplot(cont, aes(contamination, falso_alarme, fill = tipo)) +
  geom_col(width = 0.58) +
  geom_text(aes(label = pct1(falso_alarme)), vjust = -0.45, size = ANOT) +
  scale_fill_manual(values = metodo_cols, guide = "none") +
  scale_y_continuous(limits = c(0, 0.62), labels = label_percent(accuracy = 1, suffix = " %"),
                     breaks = seq(0, 0.5, 0.25),
                     expand = expansion(mult = c(0, 0.02))) +
  labs(x = "Parâmetro de contaminação", y = "Falso alarme")

# ---------------------------------------------------------------------------
# e — curvas ROC em defeito incipiente
# ---------------------------------------------------------------------------
roc <- rd("fig2e_roc.csv") |>
  mutate(cadeia = ifelse(grepl("20 kHz", banda_curta), "20 kHz", "1 kHz"),
         rotulo = sprintf("%s: AUC %s; pAUC %s", cadeia, num3(auc), num3(pauc)))

rotulo_20 <- unique(roc$rotulo[roc$cadeia == "20 kHz"])
rotulo_1  <- unique(roc$rotulo[roc$cadeia == "1 kHz"])
roc <- roc |> mutate(rotulo = factor(rotulo, levels = c(rotulo_20, rotulo_1)))

p_e <- ggplot(roc, aes(fpr, tpr, colour = rotulo, linetype = rotulo)) +
  annotate("rect", xmin = 0, xmax = 0.10, ymin = 0, ymax = 1,
           fill = pal[["orange_wash"]], alpha = 0.13) +
  geom_abline(linewidth = 0.3, linetype = "22", colour = pal[["neutral_mid"]]) +
  geom_line(linewidth = 0.45) +
  annotate("text", x = 0.24, y = 0.82, hjust = 0, size = ANOT,
           colour = "#8a5a00", label = "falso alarme até 10 %") +
  scale_colour_manual(values = setNames(c(pal[["signal_blue"]], pal[["accent_red"]]),
                                        c(rotulo_20, rotulo_1))) +
  scale_linetype_manual(values = setNames(c("solid", "31"), c(rotulo_20, rotulo_1))) +
  scale_x_continuous(limits = c(0, 1), breaks = seq(0, 1, 0.5), labels = eixo_decimal) +
  scale_y_continuous(limits = c(0, 1), breaks = seq(0, 1, 0.5), labels = eixo_decimal) +
  coord_fixed() +
  labs(x = "Falso alarme", y = "Detecção") +
  theme(legend.position = "inside", legend.position.inside = c(0.98, 0.14),
        legend.justification = c(1, 0.5), legend.direction = "vertical",
        legend.key.height = unit(1.9, "mm"), legend.key.width = unit(2.8, "mm"),
        legend.spacing.y = unit(0.2, "mm"))

# ---------------------------------------------------------------------------
# montagem: integração (a, b) | validação (c) | modelo (d, e)
# ---------------------------------------------------------------------------
fig2 <- (p_a | p_b) / (p_c | p_d | p_e) +
  plot_layout(heights = c(1, 1.05), widths = c(1)) +
  plot_annotation(tag_levels = "a") &
  tag_theme

save_pub(fig2, file.path(out_dir, "fig2_metodologia"),
         width_mm = 183, height_mm = 112)
