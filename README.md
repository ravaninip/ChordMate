---
name: ChordMate
description: Plugin VST3/AU per Ableton Live 12 che suggerisce accordi e melodie e aiuta con il MIDI
stack: C++17, JUCE 8, CMake
language: it
---

# ChordMate

Assistente di arrangiamento per **Ableton Live 12** (VST3 su Windows/macOS, AU su macOS).
Suggerisce accordi e melodie con la teoria musicale (nessuna rete, nessuna AI: tutto offline e ripetibile)
e porta il risultato in Live come clip MIDI.

## Come funziona con Live

I plugin VST/AU **non possono inviare MIDI** ad altre tracce di Live (limite di Live, non del plugin).
Per questo ChordMate lavora così:

| Direzione | Come |
|---|---|
| Live → ChordMate | Su una traccia con ChordMate imposta **MIDI From** sulla traccia da analizzare. Il plugin riconosce accordo e tonalità e può registrare le note. |
| ChordMate → Live | **Trascina** i riquadri *Accordi / Melodia / Cattura .mid* su una traccia MIDI di Live: nasce una clip. (Clic semplice = *Salva con nome*.) |

ChordMate è caricato come strumento (ha un piccolo synth interno per l'anteprima). Se non vuoi sentire il
MIDI in ingresso lascia spenta l'opzione *Suona anche il MIDI in ingresso*.

## Cosa fa

- **Tonalità e scala**: 11 scale/modi (maggiore, minore naturale/armonica/melodica, modi, pentatoniche, blues).
- **Genere di riferimento** (11 macro-generi: Universale, Pop, Rock, Jazz, R&B/Neo-soul, Elettronica, Hip-hop/Lo-fi,
  Cinematico, Classico, Latin/Bossa, Ambient). Sceglierlo cambia in un colpo: preset tipici del genere (in numeri romani),
  peso dei suggerimenti e degli accordi "colore" (es. ♭VII nel rock, ii-V e sostituzioni di tritono nel jazz, mediante
  cromatiche nel cinematico), colore degli accordi, stile ritmico, figura del basso (ottavi, walking, sincopato, bossa,
  ottave), ritmo/swing/ripetizione del motivo della melodia, ensemble e poliarmonia.
- **Colore degli accordi**: triadi, settime o estese; la 9ª si aggiunge solo se è nella scala (altrimenti 7♭9 sulle dominanti).
- **Suggerimenti accordo successivo**: dopo l'accordo selezionato propone i 6 più sensati, con grado romano e motivazione
  (cadenza perfetta, plagale, d'inganno, prestiti modali, dominanti secondarie, sostituzione di tritono...).
  Lo slider *Avventura* decide quanti accordi "colore" compaiono (0 = solo diatonici).
- **Progressioni**: preset per genere o generazione casuale ripetibile; **Rearmonizza** sostituisce alcuni accordi con
  altri della stessa funzione, dominanti secondarie o sostituti di tritono (primo e ultimo restano).
- **Voice leading**: gli accordi sono rivoltati per muoversi il meno possibile; stili: pieni, arpeggio 1/16,
  arpeggio su-giù 1/8, pulsante 1/8, contrattempo; basso opzionale.
- **Melodia a macro-controlli**: *Energia* (quante note, quanto corte e accentate), *Movimento* (gradi congiunti ↔ salti),
  *Tensione* (note d'accordo ↔ appoggiature, 9ª/13ª, cromatismi), *Contorno* (libero, arco, ascendente, discendente,
  onda) e *Registro*. Costruita a frasi di 4 battute: motivo, ripetizione trasportata sul nuovo accordo, variante, cadenza.
  I controlli ritoccano la stessa melodia; *Nuova melodia* ne genera un'altra.
- **Arrangiamento e poliarmonia**: pianoforte/pad, quartetto d'archi, orchestra d'archi, orchestra sinfonica
  (archi, corni, ottoni gravi, timpani, legni), coro SATB, ensemble di ottoni. Condotta delle parti a 4 voci
  (terza e settima sempre presenti, niente quinte/ottave parallele), ostinati di viole e violini con gli stili ritmici.
  Poliarmonia: *Estensioni* (9ª, 11ª, 13ª in scala) o *Poliaccordi* (una triade diversa sopra l'accordo, es. G/F =
  Fmaj7(9,♯11)), suonata da legni o archi divisi. **Arrangiamento .mid** è un file multitraccia: trascinato nello spazio
  vuoto sotto le tracce di Live crea una traccia per sezione (melodia compresa).
- **Cattura MIDI**: registra ciò che arriva, quantizza (1/4–1/32, forza regolabile), porta le note stonate in scala,
  e **armonizza**: propone la progressione che sta meglio sotto la melodia (Viterbi su aderenza + transizioni).
- **Riconoscimento live**: accordo che stai tenendo premuto e tonalità probabile (profili di Krumhansl) dalle ultime note.
- Lo stato (progressione, melodia, cattura, impostazioni, dimensione della finestra) è salvato nel progetto di Live.
- **Finestra ridimensionabile** (angolo in basso a destra, dal 60% al 200%): all'apertura si adatta allo schermo.
  La registrazione continua anche se chiudi la finestra del plugin.

## Compilare

**Windows:** vedi `GUIDA_WINDOWS.md` (script `build-windows.ps1` che compila e installa con un comando).

Servono CMake ≥ 3.22 e un compilatore C++17 (Visual Studio 2022 su Windows, Xcode su macOS). JUCE viene scaricato da GitHub.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure   # test del motore (oltre 21.000 controlli)
```

Prova del plugin senza host (processor + editor, 41 controlli, salva due screenshot dell'interfaccia):

```bash
cmake -B build -DCHORDMATE_BUILD_PLUGIN_TESTS=ON
cmake --build build --target chordmate_plugin_smoke
./build/chordmate_plugin_smoke screenshot.png
```

Il plugin compilato è in `build/ChordMate_artefacts/Release/VST3/ChordMate.vst3` (e `AU/` su macOS).
Con `-DCHORDMATE_COPY_AFTER_BUILD=ON` viene copiato nella cartella plugin dell'utente.

- **Windows**: copia `ChordMate.vst3` in `C:\Program Files\Common Files\VST3` e in Live: *Preferenze → Plug-ins → Rescan*.
- **macOS**: copia in `~/Library/Audio/Plug-Ins/VST3` (o `Components` per AU). Il binario non è firmato/notarizzato:
  `xattr -dr com.apple.quarantine ChordMate.vst3` se macOS lo blocca.

Su GitHub il workflow `.github/workflows/build.yml` compila Windows e macOS (universale) e pubblica i plugin come artefatti.

## Gestione con GitHub

| Cosa vuoi fare | Comando |
|---|---|
| Salvare e caricare le modifiche | `git add -A` · `git commit -m "descrizione"` · `git push` |
| Vedere l'esito della build | scheda **Actions** del repository (o `gh run list`) |
| Scaricare il plugin compilato da GitHub | `gh run download -n ChordMate-windows` |
| Pubblicare una versione | aggiorna `VERSION` in `CMakeLists.txt`, poi `git tag v0.2.0` · `git push --tags`: nasce una **Release** con gli zip Windows e macOS |

Ogni push su `main` compila e testa su Windows e macOS; un test fallito blocca la Release.

## Struttura

```
Source/Engine/     motore di teoria in C++ puro (testabile senza JUCE)
  Theory.*         scale, accordi, suggerimenti, progressioni, voicing, basso, melodia, quantizzazione, armonizzazione
  Style.*          generi, preset in numeri romani, poliaccordi, condotta delle parti SATB, arrangiamento per ensemble
  MidiWriter.*     file MIDI standard (formato 0, 480 PPQ, tempo e metrica)
  Project.*        stato completo + serializzazione testuale
Source/            plugin JUCE (processor, editor, synth di anteprima, widget)
Tests/             test del motore
```

## Limiti noti

- L'anteprima suona al tempo dell'host (o 120 BPM); non è sincronizzata alla posizione del trasporto.
- La registrazione usa la posizione dell'host se Live è in play, altrimenti un clock interno: per risultati
  precisi registra con Live in play e il metronomo attivo.
- Le clip importate da file MIDI prendono la lunghezza dalla fine della traccia (arrotondata alla battuta):
  se Live la mostra diversa, regola il loop della clip.
- Interfaccia testata solo in compilazione su Linux; la prova nell'host reale (Live 12) va fatta sulla tua macchina.
