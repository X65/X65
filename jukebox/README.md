# SGU-Tracker Jukebox

[SGU-Tracker](https://tracker.x65.zone/) can play a playlist, not just a single
song: it loads a list of tunes and plays them one after another, the way a
jukebox would. Every tune is imported on the fly and played on the
[SGU-1](https://github.com/X65/SGU-1) sound chip model, so the jukebox shows how
music written for other machines (C64 SID, Atari POKEY, AdLib/OPL, Yamaha FM,
Amiga modules, MIDI and more) sounds on the X65.

## Listen online

The playlist in this directory, [`play.lst`](play.lst), can be played in the
browser, shuffled, with no installation:

**<https://tracker.x65.zone/?playlist=https://raw.githubusercontent.com/X65/X65/refs/heads/main/jukebox/play.lst&shuffle&play>**

- `playlist=` is the URL of the playlist file to load.
- `shuffle` plays the tunes in random order.
- `play` starts playback right away.

Any playlist reachable over HTTPS can be played the same way by changing the
`playlist=` URL.

## Playlist format

A playlist is a plain text file with one tune per line. An entry is either a
full URL or a path into one of the music archives SGU-Tracker knows about, for
example:

```text
hvsc:/MUSICIANS/H/Hubbard_Rob/Monty_on_the_Run.sid
asma:/Composers/Janusz%20Pelc/Lasermania%20%281990%29.sap&song=2
reality:/Modules/RAD-v1/Void/Void%20-%20Alloyrun.rad
https://ctrl-alt-rees.com/archive/yellowantphil.com/wacky_wheels/downloads/music/dream.mid
```

Special characters in paths are URL-encoded, and `&song=N` selects a sub-song
from a multi-song file.

## Ranked additions

The 100 entries added in September 2026 are recorded in
[`top100-rankings.tsv`](top100-rankings.tsv). The list draws from four different
rankings to cover SID, module, VGM and MIDI music:

- `hvsc`: the [HVSC fan vote](https://remix64.com/supporting-pages/hvscs-top-100-sids-in-2000.html)
  from 2000. These are the ranked files SGU-Tracker currently lists that were
  not already in the jukebox.
- `modland`: files whose bytes match entries in [ModArchive's most downloaded
  modules chart](https://modarchive.org/index.php?query=tophits&request=view_chart).
- `vgmrips`: one music track from each selected [top-rated VGMRips
  pack](https://vgmrips.net/packs/top). `source_rank` is the soundtrack pack's
  chart position, not a ranking of that track. The positions came from cached
  chart pages with different crawl dates, so they are historical snapshots.
- `bitmidi`: distinct songs from [BitMidi's play-count
  ranking](https://bitmidi.com/api/midi/all?page=0&pageSize=50&orderBy=plays)
  that are also in SGU-Tracker's `Popular/` directory.

The TSV's `added_order` is playlist order; it is not a global rank. Rankings
from different archives use different measures. `count` is downloads for
ModArchive, plays for BitMidi, and empty for the other two. Source charts may
change after this selection. All 100 source files were fetched, imported and
saved as native songs with SGU-Tracker 0.11.0. That check establishes that the
files load; it does not assess how the converted audio sounds.

## Polish composer additions

The next 50 entries are listed in
[`polish50-attribution.tsv`](polish50-attribution.tsv). They contain 15 MIDI
files from Mutopia, four OPL recordings of Chopin works, 16 Atari SAP files,
and 15 MOD/XM tracker modules. The original compositions are credited to four
Polish musicians: [Fryderyk Chopin](https://culture.pl/en/artist/fryderyk-chopin-frederic-chopin),
[Janusz Pelc](https://culture.pl/pl/tworca/janusz-pelc),
[Piotr Bendyk (XTD)](https://demozoo.org/sceners/1532/), and
[Adam Skorupa (Scorpik)](https://demozoo.org/sceners/233/).

The source library's composer folders identify the MIDI, SAP, MOD and XM
authors. The selected SAP files also contain unambiguous `AUTHOR` headers. The
OPL recordings are arrangements of Chopin compositions; their archive titles
credit Chopin as the original composer. The TSV links the nationality evidence
for each entry. This batch is a selection across formats, not a numbered
popularity chart. Only `fife.xm` has a verified matching file in
[ModArchive's download chart](https://modarchive.org/index.php?query=tophits&request=view_chart):
it was number 382 with 13,539 downloads when checked. All 50 source files were
fetched and imported into native songs with SGU-Tracker 0.11.0; playback sound
was not assessed.

## Test corpus additions

The next 112 entries come from SGU-Tracker's own test corpus, the
third-party tunes in the tracker's `tests/tunes` directory that its
importers are checked against. They are listed in
[`testcorpus-validation.tsv`](testcorpus-validation.tsv). Each corpus file
was traced to a path in one of the archives SGU-Tracker browses: 33 from
modland, 27 from ASMA, 25 from the trackers' example songs, 18 from
VGMRips, seven from HVSC and two from Reality. For 102 of them the archive
file is byte-identical to the corpus file. For the other 10 it is a
different revision of the same piece, such as a newer rip or a changed
title or credit tag; the TSV's `match` column says which.

Every entry was imported with SGU-Tracker 0.11.0 and played as part of a
playlist. For 91 of them the first 60 seconds were also rendered by the
original format's reference player (sidplayfp, libopenmpt, libgme,
vgm2wav, Furnace, or the tracker's AdLib Tracker 2, klystrack and AHX
reference renderers) and compared with the SGU-1 render. `chroma` is the
average per-frame agreement of the two renders' pitch-class content, which
does not depend on timbre: 1.0 means the same notes at the same time.
`envelope` is the correlation of their loudness over time. 64 of the 91
reach a chroma of 0.9 or more. Both numbers are low where a song opens
quietly or its tempo drifts slightly, so they point to entries worth
hearing rather than proving an entry wrong. The GoatTracker, Raster Music
Tracker and extended klystrack songs, two digital-sample SAP files, and one
RAD file whose reference render came out silent have no scores.

Ten more candidates were dropped. Five were test or demo pieces rather than
music. One was a SAP file whose sub-songs are all in-game loops of 10
seconds or less. Four had chroma well below the rest at both the start and
later in the song: `super mario.a2m`, the Bombaman title screen, Lagrange
Point's "Aqueduct" and `Captain_Future_Preview.sid`. Zybex and UFO Hunt
use `&song=` to select their main sub-song, since their first sub-song is a
short jingle. None of this measures how the converted audio sounds.

## Contributing

### Adding tunes to `play.lst`

Anyone can submit tunes for the jukebox. We are looking for pieces that show
off what the SGU-1 chip and SGU-Tracker can do: tunes that import and play
well, and that make the chip sound good.

Submit them either way:

- email the entries to [tracker@xiaoka.com](mailto:tracker@xiaoka.com), or
- open a pull request against [`jukebox/play.lst`](https://github.com/X65/X65/blob/main/jukebox/play.lst)
  in the [X65/X65](https://github.com/X65/X65) repository.

Please check that every entry plays in SGU-Tracker before sending it.

### Submitting a themed playlist

The same two routes (email or pull request) work for a whole separate playlist
with a theme of its own (one composer, one platform, one game series, one genre...). Accepted
playlists are hosted in this directory next to `play.lst` and can be played
online with the same link, pointing `playlist=` at the new file:

```text
https://tracker.x65.zone/?playlist=https://raw.githubusercontent.com/X65/X65/refs/heads/main/jukebox/<name>.lst&shuffle&play
```
