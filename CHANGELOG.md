# Changelog

## [1.5.1]

### Changed

- Improved native SwipeAway behavior
- Fixed JSON serialization of singular transforms
- Build improvements
- Other bug fixes

## [1.5.0]

This release adds support for version 1.5 of the APL specification.

### Added

- New Math functions: Math.isNaN, Math.isInf and Math.isFinite
- New Array functions: Array.indexOf, Array.range, Array.slice
- New String function: String.length
- Report visible children from Sequence and GridSequence components
- DropShadow filter for AVG
- Accessibility: add fontScale, screenMode, screenReader, and timing properties to environment.
- Accessibility: Added actions and role to base component.
- Extensions: extensions can provide custom image filters
- Support bind in AVG
- Report when the visual context is dirty
- Alpha feature: handle scrolling and paging in the APL Core library
- Alpha feature: scroll snapping
- Alpha feature: custom transitions for Pager

### Changed

- Updated reported APL version to 1.5
- Accessibility: Update accessibilityLabel property to be dynamic
- Update AVG inflation to support the multi-child inflation rules
- Update gesture processing to consider applied transforms
- Bug fixes
- Build improvements

See README.md for more details on alpha features.



## [1.4.1]

This is a bug fix release of apl-core-library.

### Added

- (Alpha) Report visible children from Sequence and GridSequence components. See README.md for more details on alpha
          features.

### Changed

- Fixed visual context reporting for multi-child components
- Fixed visitor handling in pager component
- onPress is now correctly handled when a touchable component is transformed
- Improved reporting when attempting to update non-dynamic properties
- Fixed resource lookups during dependency propagation of data-binding context
- Fixed windows build
- Stop exposing context.h externally



## [1.4.0]

This release adds support for version 1.4 of the APL specification.

### Added

- New components: EditText and GridSequence
- New AVG features (e.g. text support) and improvements
- Tick handlers
- onDown, onMove, and onUp event handlers to touchable components (e.g VectorGraphic and TouchWrapper)
- Gesture support
- Named sequencers for commands in order to control which commands can execute in parallel
- Transcendental math functions
- Misc updates to component properties

### Changed

- Updated reported APL version to 1.4
- Bug fixes
- Build improvements

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


