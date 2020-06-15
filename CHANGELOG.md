# Changelog

## [1.3.3]

This release is a bug fix release of apl-core-library

### Added

- Added support for memory checks with valgrind during unit testing

### Changed

- Fixed windows build
- Fixed a bug in Math.random implementation that caused random numbers not to use the full range of possible values


## [1.3.2]

This release fixes a performance regression found in release 1.3.1.

### Changed

- Optimized layout during scrolling

## [1.3.1]

This release is a bug fix release of apl-core-library.

### Added

- dynamicIndexList: Allow data source bounds to be grown. Previously, they could only be shrunk.
- dynamicIndexList: Trigger a RuntimeError when a response to a LoadIndexListData request times out
- dynamicIndexList: Trigger a RuntimeError when a directive is buffered longer than a specified duration

### Changed

- dynamicIndexList: improved handling of delayed responses
- Expose settings to the runtime before a document is fully inflated
- Better handling of spacing when components change dynamically
- Better handling of empty item inflation
- Fixed memory leak
- Minor bug fixes

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


