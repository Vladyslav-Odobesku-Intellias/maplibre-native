#include <mbgl/test/fake_file_source.hpp>
#include <mbgl/test/util.hpp>

#include <mbgl/gfx/dynamic_texture_atlas.hpp>
#include <mbgl/map/transform.hpp>
#include <mbgl/renderer/image_manager.hpp>
#include <mbgl/renderer/tile_parameters.hpp>
#include <mbgl/style/custom_vector_tile_loader.hpp>
#include <mbgl/text/glyph_manager.hpp>
#include <mbgl/tile/custom_vector_tile.hpp>
#include <mbgl/util/run_loop.hpp>

#include <memory>

using namespace mbgl;
using namespace mbgl::style;

class CustomVectorTileTest {
public:
    util::SimpleIdentity uniqueID;
    std::shared_ptr<FileSource> fileSource = std::make_shared<FakeFileSource>();
    TransformState transformState;
    util::RunLoop loop;
    std::shared_ptr<ImageManager> imageManager = ImageManager::create();
    std::shared_ptr<GlyphManager> glyphManager = std::make_shared<GlyphManager>();
    gfx::DynamicTextureAtlasPtr dynamicTextureAtlas;

    TileParameters tileParameters;

    CustomVectorTileTest()
        : tileParameters{.pixelRatio = 1.0,
                         .debugOptions = MapDebugOptions(),
                         .transformState = transformState,
                         .fileSource = fileSource,
                         .mode = MapMode::Continuous,
                         .annotationManager = {},
                         .imageManager = imageManager,
                         .glyphManager = glyphManager,
                         .prefetchZoomDelta = 0,
                         .threadPool = {Scheduler::GetBackground(), uniqueID},
                         .dynamicTextureAtlas = dynamicTextureAtlas} {}
};

TEST(CustomVectorTileLoader, CancelsSharedCanonicalTileAfterLastConsumer) {
    CustomVectorTileTest test;

    int fetchCount = 0;
    int cancelCount = 0;
    CustomVectorTileLoader loader([&](const CanonicalTileID&) { ++fetchCount; },
                                  [&](const CanonicalTileID&) { ++cancelCount; });

    auto loaderMailbox = std::make_shared<Mailbox>();
    ActorRef<CustomVectorTileLoader> loaderActor(loader, loaderMailbox);

    const OverscaledTileID canonicalTileID(0, 0, 0, 0, 0);
    const OverscaledTileID wrappedTileID(0, 1, 0, 0, 0);
    CustomVectorTile canonicalTile(canonicalTileID, "source", test.tileParameters, loaderActor);
    CustomVectorTile wrappedTile(wrappedTileID, "source", test.tileParameters, loaderActor);

    auto canonicalMailbox = std::make_shared<Mailbox>();
    auto wrappedMailbox = std::make_shared<Mailbox>();
    ActorRef<CustomVectorTile> canonicalTileActor(canonicalTile, canonicalMailbox);
    ActorRef<CustomVectorTile> wrappedTileActor(wrappedTile, wrappedMailbox);

    loader.fetchTile(canonicalTileID, canonicalTileActor);
    loader.fetchTile(wrappedTileID, wrappedTileActor);
    EXPECT_EQ(fetchCount, 1);

    loader.cancelTile(canonicalTileID);
    EXPECT_EQ(cancelCount, 0);

    // Fetching an existing consumer again must not start a duplicate canonical request.
    loader.fetchTile(wrappedTileID, wrappedTileActor);
    EXPECT_EQ(fetchCount, 1);

    loader.removeTile(wrappedTileID);
    EXPECT_EQ(cancelCount, 1);

    loader.fetchTile(canonicalTileID, canonicalTileActor);
    EXPECT_EQ(fetchCount, 2);
}

TEST(CustomVectorTileLoader, InvalidatingSharedCanonicalTileCancelsOnce) {
    CustomVectorTileTest test;

    int cancelCount = 0;
    CustomVectorTileLoader loader([](const CanonicalTileID&) {}, [&](const CanonicalTileID&) { ++cancelCount; });

    auto loaderMailbox = std::make_shared<Mailbox>();
    ActorRef<CustomVectorTileLoader> loaderActor(loader, loaderMailbox);

    const OverscaledTileID canonicalTileID(0, 0, 0, 0, 0);
    const OverscaledTileID wrappedTileID(0, 1, 0, 0, 0);
    CustomVectorTile canonicalTile(canonicalTileID, "source", test.tileParameters, loaderActor);
    CustomVectorTile wrappedTile(wrappedTileID, "source", test.tileParameters, loaderActor);

    auto canonicalMailbox = std::make_shared<Mailbox>();
    auto wrappedMailbox = std::make_shared<Mailbox>();
    ActorRef<CustomVectorTile> canonicalTileActor(canonicalTile, canonicalMailbox);
    ActorRef<CustomVectorTile> wrappedTileActor(wrappedTile, wrappedMailbox);

    loader.fetchTile(canonicalTileID, canonicalTileActor);
    loader.fetchTile(wrappedTileID, wrappedTileActor);
    loader.invalidateTile(canonicalTileID.canonical);

    EXPECT_EQ(cancelCount, 1);
}
