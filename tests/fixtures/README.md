# Legacy save fixture

`v06-save.bin` was generated using the v0.6 source's actual `encode()` implementation (save format 4), not by changing a newer save's version number.

It contains a five-player Side right game (old layout index 5), named profile Alan, a renamed generic profile Brenda, assigned seats, partner damage 17 from player C, timer remaining 1234, and save sequence 42. The v0.7 test checks layout migration to index 3, profile promotion, and preserved game data.
