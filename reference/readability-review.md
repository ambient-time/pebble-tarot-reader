# Tarot Reader readability review

## Baseline, 0.1.0

Luke reported that color, contrast, element size, and the overall experience were hard to use on his Time 2. Installation and menu launch were verified on the physical Emery watch, firmware 4.36.2. A new physical screenshot attempt could not connect to the phone. Native Emery interaction was repeated before editing.

Button user -> choose a spread -> reveal a card -> read its meaning. Success means a visible card, its full name and orientation, readable meaning text, and a predictable Back path.

Observed: 14-pixel font requests make card names and instructions too small. Meaning text uses 18, and spends its first viewport on repeated headings rather than the interpretation. The busy green field also carries the reading text. A 104 x 168 card occupies only 52% of the 200-pixel-wide Time 2 display, with no way to enlarge its details. The overview uses tiny numbered boxes. Continue reading appears even with no saved reading.

Measured from source: a one-card draw takes three Select presses to reveal (menu -> overview -> back -> artwork), then another to read. The overview adds no choice for a single card. No timing or physical reading-speed measurements have been made.

## Changes to verify

- Larger menu and reading type, black text on a plain white reading page, explicit Select prompts. High confidence in dimensions; physical readability requires Luke's follow-up. Tradeoff: fewer words fit per page. Acceptance: complete text can be scrolled, without obscured lines or clipped controls.
- Show Continue only when a reading exists. Enlarge spread boxes where space allows, retain the ten-card map and numbered selection. Acceptance: all positions remain reachable with Up/Down, without relying on color to identify selection.
- Add an enlarged artwork view with vertical panning. Tradeoff: one additional optional mode and resource memory. Acceptance: Back restores the exact card, panning reaches both ends, and Select still reaches its meaning.
- Skip the overview for a single card. Before: menu -> overview -> card back -> reveal. After: menu -> card back -> reveal. Back returns to menu; multi-card overviews remain. Discovery, reveal control, saved state, and meanings are preserved. Acceptance: two Select presses reveal a one-card draw, and saved single-card readings resume directly.

Evidence levels: source dimensions are measured; screen appearance is observed; improved comfort is an assumption until checked on the watch. No pointer hit-target or Fitts-law claims apply to the fixed hardware buttons.

Guidance: https://developer.rebble.io/guides/design-and-interaction/recommended/ recommends a minimum font size of 18 for smaller elements and 28 for larger items.

## Verified result, 0.2.0

All six native emulator targets pass reveal, scroll, saved-reading, Back, detail panning, and the single-card shortcut. Complete card pixels and enlarged top/bottom crops match their own rasterized sources; reversed cards are included. Original art and historical text remain intact.

Menus request 24-pixel bold type on smaller displays and 28 on larger displays; meanings use 24, and instructions use 18 bold. Text foreground/background values are black and white. Digital RGB contrast is 21:1; this is not a measurement of the reflective physical screen. Colored decoration is restricted to card borders.

The small round display has its own full-card asset and reserves room for the circular footer. The enlarged Time 2 artwork is 184 pixels wide versus the 104-pixel full-card view. Both come from the original image; the enlargement does not stretch the small bitmap. The enlarged resource buffer adds 4,624 bytes of static memory. Rendering remains input-driven.

The emulator transfer initially timed out with the larger resource packet. A paced host-side transfer succeeded; this changes only the test harness. An additional inspection caught overlapping spread numbers and round-screen title/footer clipping; their layouts were corrected before the final six-target run.

The prior physical Time 2 menu capture is retained with the before/after evidence. The new build is ready locally, but the phone's developer connection did not return during three attempts. Physical comfort remains unverified and the candidate has not been installed or store-published.
