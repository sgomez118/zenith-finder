#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <filesystem>
#include <fstream>

#include "catalog_loader.hpp"
#include "engine.hpp"

using namespace engine;

TEST_CASE("Catalog Loader JSON Parsing", "[engine][catalog]") {
  const std::string test_json_path = "test_stars.json";
  std::ofstream test_file(test_json_path);
  test_file << R"({
    "data": [
        [
            "* alf CMa",
            101.28715533333335,
            -16.71611586111111,
            "A",
            -546.01,
            -1223.07,
            "A",
            379.21,
            "A",
            -5.5,
            "A",
            -1.46,
            null,
            "C",
            "** AGC    1A|PLX 1577|SBC9 416|* alf CMa A|8pc 379.21A|ADS  5423 A|CCDM J06451-1643A|CEL   1368|CSI-16  1591  1|Ci 20  396|FK5  257|GAT  474|GCRV  4392|GEN# +1.00048915A|HGAM    556|IDS 06408-1635 A|JP11  1425|LFT  486|LPM 243|LTT  2638|N30 1470|NAME Sirius A|NLTT 16953|PM 06430-1639A|PMC 90-93   186|PPM 217626|ROT  1088|SBC7   288|SKY# 11855|UBV M  12413|USNO 816|Zkh  91|uvby98 100048915 A|WDS J06451-1643A|TYC 5949-2777-1|Renson 13090|BD-16  1591A|HD  48915A|TIC 322899250|GJ 244 A|HIP 32349|CNS5 1676|* alf CMa|HR  2491|LHS   219|GC  8833|SAO 151881|*   9 CMa|BD-16  1591|HD  48915|NSV 17173|TD1  8027|UBV    6709|IRC -20105|RAFGL 1007|IRAS 06429-1639|2MASS J06450887-1642566|NAME Sirius|HIC  32349|IRAS S06429-1639|IRAS S06430-1639|AKARI-FIS-V1 J0645085-164258|WEB  6525"
        ]
    ]
})";
  test_file.close();

  auto stars = CatalogLoader::LoadStarDataFromJSON(test_json_path);
  std::filesystem::remove(test_json_path);

  REQUIRE(stars.size() == 1);
  const auto& star = stars[0];
  CHECK(star.name == "Sirius A");
  CHECK(star.catalog == "HIP");
  CHECK(star.catalog_id == 32349);
  CHECK_THAT(star.ra,
             Catch::Matchers::WithinRel(101.28715533333335, 0.00000000000001));
  CHECK_THAT(star.dec,
             Catch::Matchers::WithinRel(-16.71611586111111, 0.00000000000001));
  CHECK(star.coo_qual == 'A');
  CHECK_THAT(star.pmra, Catch::Matchers::WithinRel(-546.01, 0.01));
  CHECK_THAT(star.pmdec, Catch::Matchers::WithinRel(-1223.07, 0.01));
  CHECK(star.pm_qual == 'A');
  CHECK_THAT(star.parallax, Catch::Matchers::WithinRel(379.21, 0.01));
  CHECK(star.plx_qual == 'A');
  CHECK_THAT(star.radial_velocity, Catch::Matchers::WithinRel(-5.5, 0.1));
  CHECK(star.rvz_qual == 'A');
  CHECK_THAT(star.flux, Catch::Matchers::WithinRel(-1.46, 0.01));
  CHECK_THAT(star.flux_err, Catch::Matchers::WithinRel(0.00, 0.01));
  CHECK(star.flux_qual == 'C');
  CHECK(!star.ids.empty());
}
