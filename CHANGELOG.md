# Changelog
## [1.3.0]

Welcome to the March 2020 release of apl-core-library. This release supports version
1.3 of the APL specification.

### Added

- Added support for Windows and the Microsoft Visual C++ compiler.
- Added support for Finish Command.
- Added background property to documents.
- Added Time functions for displaying and formatting time values. Included
  three top-level bound variables for the current time: elapsedTime, localTime,
  and utcTime.
- Added export to documents.
- Added support for the Select command.
- Added document extensions, extension event handlers and extension commands.
  The data-binding environment includes a new extensions property that informs
  the APL document which extensions have been loaded.
- Added definition for new Backstack Extension to enable back navigation.
- Added handleKeyUp and handleKeydown properties to the top-level APL document and components.
- Added dynamic data containers:  LiveArray and LiveMaps.

### Changed

- Updated reported version numbers from 1.2 to 1.3.
- Bug fixes.


