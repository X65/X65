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
