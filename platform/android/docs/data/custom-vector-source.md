# Custom Vector Source

{{ activity_source_note("CustomVectorSourceActivity.kt") }}

Use `CustomVectorSource` when vector tiles need to be generated or loaded by application code instead of from a URL template. A custom tile provider is a suspending function that receives canonical `{z}/{x}/{y}` coordinates and returns uncompressed Mapbox Vector Tile (MVT) bytes.

## Implement a tile provider

Implement `CustomVectorTileProvider` and return the tile as `TileData.Mvt`. Because `fetchTile` is a suspending function, it can call other suspending APIs directly.

```kotlin
--8<-- "MapLibreAndroidTestApp/src/main/java/org/maplibre/android/testapp/activity/style/CustomVectorSourceActivity.kt:tileProvider"
```

The example generates MVT bytes locally. A production provider can retrieve the bytes from a network service, database, or other application-owned data source.

## Add the source and layer

Create the source with the provider, a coroutine scope, and its supported zoom range. A style layer that uses the source must select the layer name encoded in the MVT data with `setSourceLayer`.

```kotlin
--8<-- "MapLibreAndroidTestApp/src/main/java/org/maplibre/android/testapp/activity/style/CustomVectorSourceActivity.kt:addCustomVectorSource"
```

The source deduplicates requests for wrapped and overzoomed tiles that share the same canonical tile. Removing the source cancels its active tile requests.

## Manage the coroutine scope

The application owns the `CoroutineScope` passed to `CustomVectorSource`. Use a supervised scope on an appropriate dispatcher so one failed request does not cancel unrelated tile requests.

```kotlin
--8<-- "MapLibreAndroidTestApp/src/main/java/org/maplibre/android/testapp/activity/style/CustomVectorSourceActivity.kt:sourceScope"
```

Cancel the scope when its owner is destroyed:

```kotlin
--8<-- "MapLibreAndroidTestApp/src/main/java/org/maplibre/android/testapp/activity/style/CustomVectorSourceActivity.kt:cancelScope"
```
