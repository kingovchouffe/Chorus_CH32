import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# Charger les données CSV
df = pd.read_csv(file_path)

# Extraire les données
time = df["Second"].values  # Temps en secondes
ch1 = df["CH1"].values      # Signal d'entrée (impulsions)
ch2 = df["CH2"].values      # Signal de sortie (filtré)

# Calcul de l'intervalle de temps (supposé constant)
dt = np.mean(np.diff(time))  # Pas d'échantillonnage
fs = 1 / dt  # Fréquence d'échantillonnage

# Appliquer la FFT
N = len(time)
freqs = np.fft.fftfreq(N, dt)[:N//2]  # Fréquences positives seulement
fft_ch1 = np.fft.fft(ch1)[:N//2]  # FFT du signal d'entrée
fft_ch2 = np.fft.fft(ch2)[:N//2]  # FFT du signal filtré

# Calcul de la réponse en fréquence H(f) = FFT(CH2) / FFT(CH1)
H_f = np.abs(fft_ch2) / np.abs(fft_ch1)
H_f_db = 20 * np.log10(H_f)  # Conversion en dB

# Tracer la réponse en fréquence
plt.figure(figsize=(10, 6))
plt.plot(freqs, H_f_db, label="Réponse en fréquence", color="b")

# Mise en forme du graphe
plt.xscale("log")  # Échelle logarithmique pour les fréquences
plt.xlabel("Fréquence (Hz)")
plt.ylabel("Gain (dB)")
plt.title("Réponse en fréquence du filtre anti-repliement")
plt.grid(True, which="both", linestyle="--", linewidth=0.5)
plt.axhline(y=-3, color="r", linestyle="--", label="-3 dB (coupure)")
plt.legend()

# Afficher le graphe
plt.show()
