---
name: Guida Windows ChordMate
description: Come compilare e installare ChordMate (VST3) su Windows 10/11 e usarlo in Ableton Live 12
language: it
---

# ChordMate su Windows: guida passo passo

Due strade. La **A** non richiede di installare nulla; la **B** compila sul tuo PC.

## Strada A: GitHub compila per te (nessuna installazione)

1. Crea un account su github.com (gratis) e un **nuovo repository privato** chiamato `ChordMate`.
2. Estrai `ChordMate.zip`, apri PowerShell nella cartella `ChordMate` e lancia:
   ```powershell
   git init
   git add .
   git commit -m "ChordMate"
   git branch -M main
   git remote add origin https://github.com/TUO_UTENTE/ChordMate.git
   git push -u origin main
   ```
3. Su GitHub apri la scheda **Actions**: parte da solo il workflow *Build ChordMate* (circa 10-15 minuti).
4. A fine lavoro, nella pagina del run scarica l'artefatto **ChordMate-windows** (zip con `ChordMate.vst3`).
5. Vai al passo **Installa** qui sotto.

## Strada B: compilare sul tuo PC

Requisiti: Windows 10/11 a 64 bit, internet, circa 8 GB liberi.

1. Estrai `ChordMate.zip` in un percorso **corto**, per esempio `C:\dev\ChordMate`.
2. Apri **PowerShell** (non serve come amministratore) e installa gli strumenti, una volta sola:
   ```powershell
   winget install Kitware.CMake
   winget install Git.Git
   winget install Microsoft.VisualStudio.2022.BuildTools --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
   ```
   L'ultimo comando scarica alcuni GB. Se hai già Visual Studio 2022 con "Sviluppo di applicazioni desktop con C++", salta quel comando.
3. **Chiudi e riapri PowerShell** (serve per aggiornare il PATH).
4. Compila e installa con un solo comando:
   ```powershell
   cd C:\dev\ChordMate
   powershell -ExecutionPolicy Bypass -File .\build-windows.ps1
   ```
   Lo script controlla i prerequisiti, scarica JUCE, compila, esegue i test del motore (devono dire 0 falliti) e copia il plugin:
   - in `C:\Program Files\Common Files\VST3` se PowerShell è avviato come amministratore;
   - altrimenti in `%LOCALAPPDATA%\Programs\Common\VST3`.

La prima compilazione dura 5-15 minuti.

## Installa (solo se arrivi dalla strada A)

Copia l'intera cartella `ChordMate.vst3` in `C:\Program Files\Common Files\VST3`
(serve il permesso di amministratore).

## Attiva il plugin in Live 12

1. **Impostazioni → Plug-in**: attiva *Usa le cartelle di sistema VST3* e premi **Riesamina**
   (tieni premuto **Alt** per la scansione completa).
2. Se hai usato la cartella in `%LOCALAPPDATA%`, indicala come *cartella VST3 personalizzata*.
3. In Live trovi **ChordMate** tra i plug-in (categoria strumenti). Trascinalo su una traccia MIDI nuova.

## Primo utilizzo

1. **Genera**: scegli tonalità e scala, premi *Applica preset* o *Genera casuale*, poi *Genera melodia*. Il pulsante *Ascolta tutto* fa partire l'anteprima.
2. **Porta in Live**: trascina *Accordi .mid* (e *Melodia .mid*) su una **altra** traccia MIDI con uno strumento: nasce la clip.
3. **Analizza una traccia esistente**: sulla traccia di ChordMate imposta *MIDI From* sulla traccia da ascoltare, *Monitor: In*. Suona o fai partire Live: in alto compaiono accordo e tonalità probabile. *Registra* cattura le note, *Armonizza* propone gli accordi.

## Se qualcosa non funziona

| Sintomo | Soluzione |
|---|---|
| `cmake` non riconosciuto | Hai riaperto PowerShell dopo `winget install`? Altrimenti riavvia il PC. |
| `No CMAKE_CXX_COMPILER could be found` | Manca il carico di lavoro C++: apri *Visual Studio Installer → Modifica → Sviluppo di applicazioni desktop con C++*. |
| Errori con percorsi troppo lunghi | Sposta il progetto in `C:\dev\ChordMate` ed elimina la cartella `build`. |
| Download di JUCE fallisce | Controlla internet/proxy aziendale; riprova (`git` deve poter raggiungere github.com). |
| Live non vede il plugin | Riesamina con Alt; verifica le cartelle VST3 nelle impostazioni; controlla che esista `ChordMate.vst3\Contents\x86_64-win\ChordMate.vst3`. |
| Live lo segnala come non valido o va in crash | Invia il file `Log.txt` in `%APPDATA%\Ableton\Live 12.x\Preferences\` e il testo dell'errore. |
| SmartScreen/antivirus blocca il file | Il plugin non è firmato: *Altre info → Esegui comunque*, o aggiungi un'eccezione. |

## Nota

Non ho potuto provare la compilazione su Windows né il plugin dentro Live: il codice compila e supera i test su Linux. Alla prima prova su Windows potrebbe emergere qualche avviso o errore specifico del compilatore Microsoft: se succede, incollami l'ultimo blocco di messaggi e lo correggo.
