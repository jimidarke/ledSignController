# Legal Notice and Protocol Documentation

## Protocol Documentation Source

This project implements the **Alpha Sign Communications Protocol**, which is officially
documented and freely published by the manufacturer for third-party integration.

### Official Documentation

| Document | Source | URL |
|----------|--------|-----|
| Alpha Sign Communications Protocol | Alpha-American | https://www.alpha-american.com/alpha-manuals/M-Protocol.pdf |
| Protocol Document (pn 9708-8061) | Alpha-American | https://alpha-american.com/9708-8061.html |
| Technical Manuals Index | Alpha-American | https://alpha-american.com/Technical-Manuals.html |
| Support Documents | Adaptive Displays | http://support.adaptivedisplays.com/Documentation/ |

The protocol documentation states:
> "Alpha's Protocol documentation allows complete control of Alpha electronic message
> displays. The protocol allows signs to communicate directly with cash registers,
> point-of-purchase displays, programmable logic controls, desktop PCs, or almost
> any device with a serial port."

This indicates the manufacturer **explicitly intends** for third parties to implement
the protocol for integration purposes.

## Independent Implementation

This ESPHome component is an **independent implementation** created from the publicly
available protocol documentation listed above. No proprietary code was copied or
reverse-engineered.

## Other Open Source Implementations

Multiple open-source implementations of the Alpha Protocol exist, confirming the
protocol's public nature and the legality of implementing it:

| Project | Language | License | Repository |
|---------|----------|---------|------------|
| alphasign | Python | Apache 2.0 | https://github.com/msparks/alphasign |
| betabrite | Ruby | MIT | https://github.com/depili/betabrite |
| python-alphasign | Python 3 | MIT | https://github.com/prototux/python-alphasign |
| betabrite | Python | BSD | https://github.com/jonathankoren/betabrite |
| BetaBrite LED Sign API | C#/.NET | CPOL | https://www.codeproject.com/Articles/9886/BetaBrite-LED-Sign-API |

## Legal Basis

Implementing a publicly documented communication protocol is legal under established
case law:

1. **Sega Enterprises v. Accolade (1992)** - Reverse engineering for interoperability
   purposes constitutes fair use.

2. **Sony Computer Entertainment v. Connectix (2000)** - Creating compatible
   implementations through clean-room engineering is legal.

3. **Google LLC v. Oracle America (2021)** - APIs and functional specifications are
   not copyrightable creative expression; implementing them is fair use.

This implementation is **even more clearly legal** because:
- No reverse engineering was required (protocol is documented)
- The manufacturer publishes documentation specifically for third-party integration
- Multiple other open-source implementations already exist under permissive licenses

## Trademarks

- **BetaBrite** is a registered trademark of Adaptive Micro Systems LLC.
- **Alpha** is a trademark of Adaptive Micro Systems LLC.
- **Alpha-American** is a trademark of Alpha-American Programmable Signs.

Use of these trademarks in this project is for identification and interoperability
purposes only. This project is not affiliated with, endorsed by, or sponsored by
Adaptive Micro Systems LLC, Alpha-American, or any LED sign manufacturer.

## License

This project is released under the MIT License. See [LICENSE](LICENSE) for details.
