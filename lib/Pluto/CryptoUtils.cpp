// Credits: https://github.com/bluesadi/Pluto

#include "Pluto/CryptoUtils.h"

#include "llvm/ADT/Twine.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Debug.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdint.h>

// Stats
#define DEBUG_TYPE "CryptoUtils"

using namespace Pluto;
using namespace llvm;

namespace Pluto {
llvm::ManagedStatic<CryptoUtils> cryptoutils;
}

static const uint32_t AES_RCON[10] = {0x01000000UL, 0x02000000UL, 0x04000000UL, 0x08000000UL, 0x10000000UL,
                               0x20000000UL, 0x40000000UL, 0x80000000UL, 0x1b000000UL, 0x36000000UL};

static const uint32_t AES_PRECOMP_TE0[256] = {
    0xc66363a5UL, 0xf87c7c84UL, 0xee777799UL, 0xf67b7b8dUL, 0xfff2f20dUL, 0xd66b6bbdUL, 0xde6f6fb1UL, 0x91c5c554UL,
    0x60303050UL, 0x02010103UL, 0xce6767a9UL, 0x562b2b7dUL, 0xe7fefe19UL, 0xb5d7d762UL, 0x4dababe6UL, 0xec76769aUL,
    0x8fcaca45UL, 0x1f82829dUL, 0x89c9c940UL, 0xfa7d7d87UL, 0xeffafa15UL, 0xb25959ebUL, 0x8e4747c9UL, 0xfbf0f00bUL,
    0x41adadecUL, 0xb3d4d467UL, 0x5fa2a2fdUL, 0x45afafeaUL, 0x239c9cbfUL, 0x53a4a4f7UL, 0xe4727296UL, 0x9bc0c05bUL,
    0x75b7b7c2UL, 0xe1fdfd1cUL, 0x3d9393aeUL, 0x4c26266aUL, 0x6c36365aUL, 0x7e3f3f41UL, 0xf5f7f702UL, 0x83cccc4fUL,
    0x6834345cUL, 0x51a5a5f4UL, 0xd1e5e534UL, 0xf9f1f108UL, 0xe2717193UL, 0xabd8d873UL, 0x62313153UL, 0x2a15153fUL,
    0x0804040cUL, 0x95c7c752UL, 0x46232365UL, 0x9dc3c35eUL, 0x30181828UL, 0x379696a1UL, 0x0a05050fUL, 0x2f9a9ab5UL,
    0x0e070709UL, 0x24121236UL, 0x1b80809bUL, 0xdfe2e23dUL, 0xcdebeb26UL, 0x4e272769UL, 0x7fb2b2cdUL, 0xea75759fUL,
    0x1209091bUL, 0x1d83839eUL, 0x582c2c74UL, 0x341a1a2eUL, 0x361b1b2dUL, 0xdc6e6eb2UL, 0xb45a5aeeUL, 0x5ba0a0fbUL,
    0xa45252f6UL, 0x763b3b4dUL, 0xb7d6d661UL, 0x7db3b3ceUL, 0x5229297bUL, 0xdde3e33eUL, 0x5e2f2f71UL, 0x13848497UL,
    0xa65353f5UL, 0xb9d1d168UL, 0x00000000UL, 0xc1eded2cUL, 0x40202060UL, 0xe3fcfc1fUL, 0x79b1b1c8UL, 0xb65b5bedUL,
    0xd46a6abeUL, 0x8dcbcb46UL, 0x67bebed9UL, 0x7239394bUL, 0x944a4adeUL, 0x984c4cd4UL, 0xb05858e8UL, 0x85cfcf4aUL,
    0xbbd0d06bUL, 0xc5efef2aUL, 0x4faaaae5UL, 0xedfbfb16UL, 0x864343c5UL, 0x9a4d4dd7UL, 0x66333355UL, 0x11858594UL,
    0x8a4545cfUL, 0xe9f9f910UL, 0x04020206UL, 0xfe7f7f81UL, 0xa05050f0UL, 0x783c3c44UL, 0x259f9fbaUL, 0x4ba8a8e3UL,
    0xa25151f3UL, 0x5da3a3feUL, 0x804040c0UL, 0x058f8f8aUL, 0x3f9292adUL, 0x219d9dbcUL, 0x70383848UL, 0xf1f5f504UL,
    0x63bcbcdfUL, 0x77b6b6c1UL, 0xafdada75UL, 0x42212163UL, 0x20101030UL, 0xe5ffff1aUL, 0xfdf3f30eUL, 0xbfd2d26dUL,
    0x81cdcd4cUL, 0x180c0c14UL, 0x26131335UL, 0xc3ecec2fUL, 0xbe5f5fe1UL, 0x359797a2UL, 0x884444ccUL, 0x2e171739UL,
    0x93c4c457UL, 0x55a7a7f2UL, 0xfc7e7e82UL, 0x7a3d3d47UL, 0xc86464acUL, 0xba5d5de7UL, 0x3219192bUL, 0xe6737395UL,
    0xc06060a0UL, 0x19818198UL, 0x9e4f4fd1UL, 0xa3dcdc7fUL, 0x44222266UL, 0x542a2a7eUL, 0x3b9090abUL, 0x0b888883UL,
    0x8c4646caUL, 0xc7eeee29UL, 0x6bb8b8d3UL, 0x2814143cUL, 0xa7dede79UL, 0xbc5e5ee2UL, 0x160b0b1dUL, 0xaddbdb76UL,
    0xdbe0e03bUL, 0x64323256UL, 0x743a3a4eUL, 0x140a0a1eUL, 0x924949dbUL, 0x0c06060aUL, 0x4824246cUL, 0xb85c5ce4UL,
    0x9fc2c25dUL, 0xbdd3d36eUL, 0x43acacefUL, 0xc46262a6UL, 0x399191a8UL, 0x319595a4UL, 0xd3e4e437UL, 0xf279798bUL,
    0xd5e7e732UL, 0x8bc8c843UL, 0x6e373759UL, 0xda6d6db7UL, 0x018d8d8cUL, 0xb1d5d564UL, 0x9c4e4ed2UL, 0x49a9a9e0UL,
    0xd86c6cb4UL, 0xac5656faUL, 0xf3f4f407UL, 0xcfeaea25UL, 0xca6565afUL, 0xf47a7a8eUL, 0x47aeaee9UL, 0x10080818UL,
    0x6fbabad5UL, 0xf0787888UL, 0x4a25256fUL, 0x5c2e2e72UL, 0x381c1c24UL, 0x57a6a6f1UL, 0x73b4b4c7UL, 0x97c6c651UL,
    0xcbe8e823UL, 0xa1dddd7cUL, 0xe874749cUL, 0x3e1f1f21UL, 0x964b4bddUL, 0x61bdbddcUL, 0x0d8b8b86UL, 0x0f8a8a85UL,
    0xe0707090UL, 0x7c3e3e42UL, 0x71b5b5c4UL, 0xcc6666aaUL, 0x904848d8UL, 0x06030305UL, 0xf7f6f601UL, 0x1c0e0e12UL,
    0xc26161a3UL, 0x6a35355fUL, 0xae5757f9UL, 0x69b9b9d0UL, 0x17868691UL, 0x99c1c158UL, 0x3a1d1d27UL, 0x279e9eb9UL,
    0xd9e1e138UL, 0xebf8f813UL, 0x2b9898b3UL, 0x22111133UL, 0xd26969bbUL, 0xa9d9d970UL, 0x078e8e89UL, 0x339494a7UL,
    0x2d9b9bb6UL, 0x3c1e1e22UL, 0x15878792UL, 0xc9e9e920UL, 0x87cece49UL, 0xaa5555ffUL, 0x50282878UL, 0xa5dfdf7aUL,
    0x038c8c8fUL, 0x59a1a1f8UL, 0x09898980UL, 0x1a0d0d17UL, 0x65bfbfdaUL, 0xd7e6e631UL, 0x844242c6UL, 0xd06868b8UL,
    0x824141c3UL, 0x299999b0UL, 0x5a2d2d77UL, 0x1e0f0f11UL, 0x7bb0b0cbUL, 0xa85454fcUL, 0x6dbbbbd6UL, 0x2c16163aUL};

static const uint32_t AES_PRECOMP_TE1[256] = {
    0xa5c66363UL, 0x84f87c7cUL, 0x99ee7777UL, 0x8df67b7bUL, 0x0dfff2f2UL, 0xbdd66b6bUL, 0xb1de6f6fUL, 0x5491c5c5UL,
    0x50603030UL, 0x03020101UL, 0xa9ce6767UL, 0x7d562b2bUL, 0x19e7fefeUL, 0x62b5d7d7UL, 0xe64dababUL, 0x9aec7676UL,
    0x458fcacaUL, 0x9d1f8282UL, 0x4089c9c9UL, 0x87fa7d7dUL, 0x15effafaUL, 0xebb25959UL, 0xc98e4747UL, 0x0bfbf0f0UL,
    0xec41adadUL, 0x67b3d4d4UL, 0xfd5fa2a2UL, 0xea45afafUL, 0xbf239c9cUL, 0xf753a4a4UL, 0x96e47272UL, 0x5b9bc0c0UL,
    0xc275b7b7UL, 0x1ce1fdfdUL, 0xae3d9393UL, 0x6a4c2626UL, 0x5a6c3636UL, 0x417e3f3fUL, 0x02f5f7f7UL, 0x4f83ccccUL,
    0x5c683434UL, 0xf451a5a5UL, 0x34d1e5e5UL, 0x08f9f1f1UL, 0x93e27171UL, 0x73abd8d8UL, 0x53623131UL, 0x3f2a1515UL,
    0x0c080404UL, 0x5295c7c7UL, 0x65462323UL, 0x5e9dc3c3UL, 0x28301818UL, 0xa1379696UL, 0x0f0a0505UL, 0xb52f9a9aUL,
    0x090e0707UL, 0x36241212UL, 0x9b1b8080UL, 0x3ddfe2e2UL, 0x26cdebebUL, 0x694e2727UL, 0xcd7fb2b2UL, 0x9fea7575UL,
    0x1b120909UL, 0x9e1d8383UL, 0x74582c2cUL, 0x2e341a1aUL, 0x2d361b1bUL, 0xb2dc6e6eUL, 0xeeb45a5aUL, 0xfb5ba0a0UL,
    0xf6a45252UL, 0x4d763b3bUL, 0x61b7d6d6UL, 0xce7db3b3UL, 0x7b522929UL, 0x3edde3e3UL, 0x715e2f2fUL, 0x97138484UL,
    0xf5a65353UL, 0x68b9d1d1UL, 0x00000000UL, 0x2cc1ededUL, 0x60402020UL, 0x1fe3fcfcUL, 0xc879b1b1UL, 0xedb65b5bUL,
    0xbed46a6aUL, 0x468dcbcbUL, 0xd967bebeUL, 0x4b723939UL, 0xde944a4aUL, 0xd4984c4cUL, 0xe8b05858UL, 0x4a85cfcfUL,
    0x6bbbd0d0UL, 0x2ac5efefUL, 0xe54faaaaUL, 0x16edfbfbUL, 0xc5864343UL, 0xd79a4d4dUL, 0x55663333UL, 0x94118585UL,
    0xcf8a4545UL, 0x10e9f9f9UL, 0x06040202UL, 0x81fe7f7fUL, 0xf0a05050UL, 0x44783c3cUL, 0xba259f9fUL, 0xe34ba8a8UL,
    0xf3a25151UL, 0xfe5da3a3UL, 0xc0804040UL, 0x8a058f8fUL, 0xad3f9292UL, 0xbc219d9dUL, 0x48703838UL, 0x04f1f5f5UL,
    0xdf63bcbcUL, 0xc177b6b6UL, 0x75afdadaUL, 0x63422121UL, 0x30201010UL, 0x1ae5ffffUL, 0x0efdf3f3UL, 0x6dbfd2d2UL,
    0x4c81cdcdUL, 0x14180c0cUL, 0x35261313UL, 0x2fc3ececUL, 0xe1be5f5fUL, 0xa2359797UL, 0xcc884444UL, 0x392e1717UL,
    0x5793c4c4UL, 0xf255a7a7UL, 0x82fc7e7eUL, 0x477a3d3dUL, 0xacc86464UL, 0xe7ba5d5dUL, 0x2b321919UL, 0x95e67373UL,
    0xa0c06060UL, 0x98198181UL, 0xd19e4f4fUL, 0x7fa3dcdcUL, 0x66442222UL, 0x7e542a2aUL, 0xab3b9090UL, 0x830b8888UL,
    0xca8c4646UL, 0x29c7eeeeUL, 0xd36bb8b8UL, 0x3c281414UL, 0x79a7dedeUL, 0xe2bc5e5eUL, 0x1d160b0bUL, 0x76addbdbUL,
    0x3bdbe0e0UL, 0x56643232UL, 0x4e743a3aUL, 0x1e140a0aUL, 0xdb924949UL, 0x0a0c0606UL, 0x6c482424UL, 0xe4b85c5cUL,
    0x5d9fc2c2UL, 0x6ebdd3d3UL, 0xef43acacUL, 0xa6c46262UL, 0xa8399191UL, 0xa4319595UL, 0x37d3e4e4UL, 0x8bf27979UL,
    0x32d5e7e7UL, 0x438bc8c8UL, 0x596e3737UL, 0xb7da6d6dUL, 0x8c018d8dUL, 0x64b1d5d5UL, 0xd29c4e4eUL, 0xe049a9a9UL,
    0xb4d86c6cUL, 0xfaac5656UL, 0x07f3f4f4UL, 0x25cfeaeaUL, 0xafca6565UL, 0x8ef47a7aUL, 0xe947aeaeUL, 0x18100808UL,
    0xd56fbabaUL, 0x88f07878UL, 0x6f4a2525UL, 0x725c2e2eUL, 0x24381c1cUL, 0xf157a6a6UL, 0xc773b4b4UL, 0x5197c6c6UL,
    0x23cbe8e8UL, 0x7ca1ddddUL, 0x9ce87474UL, 0x213e1f1fUL, 0xdd964b4bUL, 0xdc61bdbdUL, 0x860d8b8bUL, 0x850f8a8aUL,
    0x90e07070UL, 0x427c3e3eUL, 0xc471b5b5UL, 0xaacc6666UL, 0xd8904848UL, 0x05060303UL, 0x01f7f6f6UL, 0x121c0e0eUL,
    0xa3c26161UL, 0x5f6a3535UL, 0xf9ae5757UL, 0xd069b9b9UL, 0x91178686UL, 0x5899c1c1UL, 0x273a1d1dUL, 0xb9279e9eUL,
    0x38d9e1e1UL, 0x13ebf8f8UL, 0xb32b9898UL, 0x33221111UL, 0xbbd26969UL, 0x70a9d9d9UL, 0x89078e8eUL, 0xa7339494UL,
    0xb62d9b9bUL, 0x223c1e1eUL, 0x92158787UL, 0x20c9e9e9UL, 0x4987ceceUL, 0xffaa5555UL, 0x78502828UL, 0x7aa5dfdfUL,
    0x8f038c8cUL, 0xf859a1a1UL, 0x80098989UL, 0x171a0d0dUL, 0xda65bfbfUL, 0x31d7e6e6UL, 0xc6844242UL, 0xb8d06868UL,
    0xc3824141UL, 0xb0299999UL, 0x775a2d2dUL, 0x111e0f0fUL, 0xcb7bb0b0UL, 0xfca85454UL, 0xd66dbbbbUL, 0x3a2c1616UL};

static const uint32_t AES_PRECOMP_TE2[256] = {
    0x63a5c663UL, 0x7c84f87cUL, 0x7799ee77UL, 0x7b8df67bUL, 0xf20dfff2UL, 0x6bbdd66bUL, 0x6fb1de6fUL, 0xc55491c5UL,
    0x30506030UL, 0x01030201UL, 0x67a9ce67UL, 0x2b7d562bUL, 0xfe19e7feUL, 0xd762b5d7UL, 0xabe64dabUL, 0x769aec76UL,
    0xca458fcaUL, 0x829d1f82UL, 0xc94089c9UL, 0x7d87fa7dUL, 0xfa15effaUL, 0x59ebb259UL, 0x47c98e47UL, 0xf00bfbf0UL,
    0xadec41adUL, 0xd467b3d4UL, 0xa2fd5fa2UL, 0xafea45afUL, 0x9cbf239cUL, 0xa4f753a4UL, 0x7296e472UL, 0xc05b9bc0UL,
    0xb7c275b7UL, 0xfd1ce1fdUL, 0x93ae3d93UL, 0x266a4c26UL, 0x365a6c36UL, 0x3f417e3fUL, 0xf702f5f7UL, 0xcc4f83ccUL,
    0x345c6834UL, 0xa5f451a5UL, 0xe534d1e5UL, 0xf108f9f1UL, 0x7193e271UL, 0xd873abd8UL, 0x31536231UL, 0x153f2a15UL,
    0x040c0804UL, 0xc75295c7UL, 0x23654623UL, 0xc35e9dc3UL, 0x18283018UL, 0x96a13796UL, 0x050f0a05UL, 0x9ab52f9aUL,
    0x07090e07UL, 0x12362412UL, 0x809b1b80UL, 0xe23ddfe2UL, 0xeb26cdebUL, 0x27694e27UL, 0xb2cd7fb2UL, 0x759fea75UL,
    0x091b1209UL, 0x839e1d83UL, 0x2c74582cUL, 0x1a2e341aUL, 0x1b2d361bUL, 0x6eb2dc6eUL, 0x5aeeb45aUL, 0xa0fb5ba0UL,
    0x52f6a452UL, 0x3b4d763bUL, 0xd661b7d6UL, 0xb3ce7db3UL, 0x297b5229UL, 0xe33edde3UL, 0x2f715e2fUL, 0x84971384UL,
    0x53f5a653UL, 0xd168b9d1UL, 0x00000000UL, 0xed2cc1edUL, 0x20604020UL, 0xfc1fe3fcUL, 0xb1c879b1UL, 0x5bedb65bUL,
    0x6abed46aUL, 0xcb468dcbUL, 0xbed967beUL, 0x394b7239UL, 0x4ade944aUL, 0x4cd4984cUL, 0x58e8b058UL, 0xcf4a85cfUL,
    0xd06bbbd0UL, 0xef2ac5efUL, 0xaae54faaUL, 0xfb16edfbUL, 0x43c58643UL, 0x4dd79a4dUL, 0x33556633UL, 0x85941185UL,
    0x45cf8a45UL, 0xf910e9f9UL, 0x02060402UL, 0x7f81fe7fUL, 0x50f0a050UL, 0x3c44783cUL, 0x9fba259fUL, 0xa8e34ba8UL,
    0x51f3a251UL, 0xa3fe5da3UL, 0x40c08040UL, 0x8f8a058fUL, 0x92ad3f92UL, 0x9dbc219dUL, 0x38487038UL, 0xf504f1f5UL,
    0xbcdf63bcUL, 0xb6c177b6UL, 0xda75afdaUL, 0x21634221UL, 0x10302010UL, 0xff1ae5ffUL, 0xf30efdf3UL, 0xd26dbfd2UL,
    0xcd4c81cdUL, 0x0c14180cUL, 0x13352613UL, 0xec2fc3ecUL, 0x5fe1be5fUL, 0x97a23597UL, 0x44cc8844UL, 0x17392e17UL,
    0xc45793c4UL, 0xa7f255a7UL, 0x7e82fc7eUL, 0x3d477a3dUL, 0x64acc864UL, 0x5de7ba5dUL, 0x192b3219UL, 0x7395e673UL,
    0x60a0c060UL, 0x81981981UL, 0x4fd19e4fUL, 0xdc7fa3dcUL, 0x22664422UL, 0x2a7e542aUL, 0x90ab3b90UL, 0x88830b88UL,
    0x46ca8c46UL, 0xee29c7eeUL, 0xb8d36bb8UL, 0x143c2814UL, 0xde79a7deUL, 0x5ee2bc5eUL, 0x0b1d160bUL, 0xdb76addbUL,
    0xe03bdbe0UL, 0x32566432UL, 0x3a4e743aUL, 0x0a1e140aUL, 0x49db9249UL, 0x060a0c06UL, 0x246c4824UL, 0x5ce4b85cUL,
    0xc25d9fc2UL, 0xd36ebdd3UL, 0xacef43acUL, 0x62a6c462UL, 0x91a83991UL, 0x95a43195UL, 0xe437d3e4UL, 0x798bf279UL,
    0xe732d5e7UL, 0xc8438bc8UL, 0x37596e37UL, 0x6db7da6dUL, 0x8d8c018dUL, 0xd564b1d5UL, 0x4ed29c4eUL, 0xa9e049a9UL,
    0x6cb4d86cUL, 0x56faac56UL, 0xf407f3f4UL, 0xea25cfeaUL, 0x65afca65UL, 0x7a8ef47aUL, 0xaee947aeUL, 0x08181008UL,
    0xbad56fbaUL, 0x7888f078UL, 0x256f4a25UL, 0x2e725c2eUL, 0x1c24381cUL, 0xa6f157a6UL, 0xb4c773b4UL, 0xc65197c6UL,
    0xe823cbe8UL, 0xdd7ca1ddUL, 0x749ce874UL, 0x1f213e1fUL, 0x4bdd964bUL, 0xbddc61bdUL, 0x8b860d8bUL, 0x8a850f8aUL,
    0x7090e070UL, 0x3e427c3eUL, 0xb5c471b5UL, 0x66aacc66UL, 0x48d89048UL, 0x03050603UL, 0xf601f7f6UL, 0x0e121c0eUL,
    0x61a3c261UL, 0x355f6a35UL, 0x57f9ae57UL, 0xb9d069b9UL, 0x86911786UL, 0xc15899c1UL, 0x1d273a1dUL, 0x9eb9279eUL,
    0xe138d9e1UL, 0xf813ebf8UL, 0x98b32b98UL, 0x11332211UL, 0x69bbd269UL, 0xd970a9d9UL, 0x8e89078eUL, 0x94a73394UL,
    0x9bb62d9bUL, 0x1e223c1eUL, 0x87921587UL, 0xe920c9e9UL, 0xce4987ceUL, 0x55ffaa55UL, 0x28785028UL, 0xdf7aa5dfUL,
    0x8c8f038cUL, 0xa1f859a1UL, 0x89800989UL, 0x0d171a0dUL, 0xbfda65bfUL, 0xe631d7e6UL, 0x42c68442UL, 0x68b8d068UL,
    0x41c38241UL, 0x99b02999UL, 0x2d775a2dUL, 0x0f111e0fUL, 0xb0cb7bb0UL, 0x54fca854UL, 0xbbd66dbbUL, 0x163a2c16UL};

static const uint32_t AES_PRECOMP_TE3[256] = {
    0x6363a5c6UL, 0x7c7c84f8UL, 0x777799eeUL, 0x7b7b8df6UL, 0xf2f20dffUL, 0x6b6bbdd6UL, 0x6f6fb1deUL, 0xc5c55491UL,
    0x30305060UL, 0x01010302UL, 0x6767a9ceUL, 0x2b2b7d56UL, 0xfefe19e7UL, 0xd7d762b5UL, 0xababe64dUL, 0x76769aecUL,
    0xcaca458fUL, 0x82829d1fUL, 0xc9c94089UL, 0x7d7d87faUL, 0xfafa15efUL, 0x5959ebb2UL, 0x4747c98eUL, 0xf0f00bfbUL,
    0xadadec41UL, 0xd4d467b3UL, 0xa2a2fd5fUL, 0xafafea45UL, 0x9c9cbf23UL, 0xa4a4f753UL, 0x727296e4UL, 0xc0c05b9bUL,
    0xb7b7c275UL, 0xfdfd1ce1UL, 0x9393ae3dUL, 0x26266a4cUL, 0x36365a6cUL, 0x3f3f417eUL, 0xf7f702f5UL, 0xcccc4f83UL,
    0x34345c68UL, 0xa5a5f451UL, 0xe5e534d1UL, 0xf1f108f9UL, 0x717193e2UL, 0xd8d873abUL, 0x31315362UL, 0x15153f2aUL,
    0x04040c08UL, 0xc7c75295UL, 0x23236546UL, 0xc3c35e9dUL, 0x18182830UL, 0x9696a137UL, 0x05050f0aUL, 0x9a9ab52fUL,
    0x0707090eUL, 0x12123624UL, 0x80809b1bUL, 0xe2e23ddfUL, 0xebeb26cdUL, 0x2727694eUL, 0xb2b2cd7fUL, 0x75759feaUL,
    0x09091b12UL, 0x83839e1dUL, 0x2c2c7458UL, 0x1a1a2e34UL, 0x1b1b2d36UL, 0x6e6eb2dcUL, 0x5a5aeeb4UL, 0xa0a0fb5bUL,
    0x5252f6a4UL, 0x3b3b4d76UL, 0xd6d661b7UL, 0xb3b3ce7dUL, 0x29297b52UL, 0xe3e33eddUL, 0x2f2f715eUL, 0x84849713UL,
    0x5353f5a6UL, 0xd1d168b9UL, 0x00000000UL, 0xeded2cc1UL, 0x20206040UL, 0xfcfc1fe3UL, 0xb1b1c879UL, 0x5b5bedb6UL,
    0x6a6abed4UL, 0xcbcb468dUL, 0xbebed967UL, 0x39394b72UL, 0x4a4ade94UL, 0x4c4cd498UL, 0x5858e8b0UL, 0xcfcf4a85UL,
    0xd0d06bbbUL, 0xefef2ac5UL, 0xaaaae54fUL, 0xfbfb16edUL, 0x4343c586UL, 0x4d4dd79aUL, 0x33335566UL, 0x85859411UL,
    0x4545cf8aUL, 0xf9f910e9UL, 0x02020604UL, 0x7f7f81feUL, 0x5050f0a0UL, 0x3c3c4478UL, 0x9f9fba25UL, 0xa8a8e34bUL,
    0x5151f3a2UL, 0xa3a3fe5dUL, 0x4040c080UL, 0x8f8f8a05UL, 0x9292ad3fUL, 0x9d9dbc21UL, 0x38384870UL, 0xf5f504f1UL,
    0xbcbcdf63UL, 0xb6b6c177UL, 0xdada75afUL, 0x21216342UL, 0x10103020UL, 0xffff1ae5UL, 0xf3f30efdUL, 0xd2d26dbfUL,
    0xcdcd4c81UL, 0x0c0c1418UL, 0x13133526UL, 0xecec2fc3UL, 0x5f5fe1beUL, 0x9797a235UL, 0x4444cc88UL, 0x1717392eUL,
    0xc4c45793UL, 0xa7a7f255UL, 0x7e7e82fcUL, 0x3d3d477aUL, 0x6464acc8UL, 0x5d5de7baUL, 0x19192b32UL, 0x737395e6UL,
    0x6060a0c0UL, 0x81819819UL, 0x4f4fd19eUL, 0xdcdc7fa3UL, 0x22226644UL, 0x2a2a7e54UL, 0x9090ab3bUL, 0x8888830bUL,
    0x4646ca8cUL, 0xeeee29c7UL, 0xb8b8d36bUL, 0x14143c28UL, 0xdede79a7UL, 0x5e5ee2bcUL, 0x0b0b1d16UL, 0xdbdb76adUL,
    0xe0e03bdbUL, 0x32325664UL, 0x3a3a4e74UL, 0x0a0a1e14UL, 0x4949db92UL, 0x06060a0cUL, 0x24246c48UL, 0x5c5ce4b8UL,
    0xc2c25d9fUL, 0xd3d36ebdUL, 0xacacef43UL, 0x6262a6c4UL, 0x9191a839UL, 0x9595a431UL, 0xe4e437d3UL, 0x79798bf2UL,
    0xe7e732d5UL, 0xc8c8438bUL, 0x3737596eUL, 0x6d6db7daUL, 0x8d8d8c01UL, 0xd5d564b1UL, 0x4e4ed29cUL, 0xa9a9e049UL,
    0x6c6cb4d8UL, 0x5656faacUL, 0xf4f407f3UL, 0xeaea25cfUL, 0x6565afcaUL, 0x7a7a8ef4UL, 0xaeaee947UL, 0x08081810UL,
    0xbabad56fUL, 0x787888f0UL, 0x25256f4aUL, 0x2e2e725cUL, 0x1c1c2438UL, 0xa6a6f157UL, 0xb4b4c773UL, 0xc6c65197UL,
    0xe8e823cbUL, 0xdddd7ca1UL, 0x74749ce8UL, 0x1f1f213eUL, 0x4b4bdd96UL, 0xbdbddc61UL, 0x8b8b860dUL, 0x8a8a850fUL,
    0x707090e0UL, 0x3e3e427cUL, 0xb5b5c471UL, 0x6666aaccUL, 0x4848d890UL, 0x03030506UL, 0xf6f601f7UL, 0x0e0e121cUL,
    0x6161a3c2UL, 0x35355f6aUL, 0x5757f9aeUL, 0xb9b9d069UL, 0x86869117UL, 0xc1c15899UL, 0x1d1d273aUL, 0x9e9eb927UL,
    0xe1e138d9UL, 0xf8f813ebUL, 0x9898b32bUL, 0x11113322UL, 0x6969bbd2UL, 0xd9d970a9UL, 0x8e8e8907UL, 0x9494a733UL,
    0x9b9bb62dUL, 0x1e1e223cUL, 0x87879215UL, 0xe9e920c9UL, 0xcece4987UL, 0x5555ffaaUL, 0x28287850UL, 0xdfdf7aa5UL,
    0x8c8c8f03UL, 0xa1a1f859UL, 0x89898009UL, 0x0d0d171aUL, 0xbfbfda65UL, 0xe6e631d7UL, 0x4242c684UL, 0x6868b8d0UL,
    0x4141c382UL, 0x9999b029UL, 0x2d2d775aUL, 0x0f0f111eUL, 0xb0b0cb7bUL, 0x5454fca8UL, 0xbbbbd66dUL, 0x16163a2cUL};

static const uint32_t AES_PRECOMP_TE4_0[256] = {
    0x00000063UL, 0x0000007cUL, 0x00000077UL, 0x0000007bUL, 0x000000f2UL, 0x0000006bUL, 0x0000006fUL, 0x000000c5UL,
    0x00000030UL, 0x00000001UL, 0x00000067UL, 0x0000002bUL, 0x000000feUL, 0x000000d7UL, 0x000000abUL, 0x00000076UL,
    0x000000caUL, 0x00000082UL, 0x000000c9UL, 0x0000007dUL, 0x000000faUL, 0x00000059UL, 0x00000047UL, 0x000000f0UL,
    0x000000adUL, 0x000000d4UL, 0x000000a2UL, 0x000000afUL, 0x0000009cUL, 0x000000a4UL, 0x00000072UL, 0x000000c0UL,
    0x000000b7UL, 0x000000fdUL, 0x00000093UL, 0x00000026UL, 0x00000036UL, 0x0000003fUL, 0x000000f7UL, 0x000000ccUL,
    0x00000034UL, 0x000000a5UL, 0x000000e5UL, 0x000000f1UL, 0x00000071UL, 0x000000d8UL, 0x00000031UL, 0x00000015UL,
    0x00000004UL, 0x000000c7UL, 0x00000023UL, 0x000000c3UL, 0x00000018UL, 0x00000096UL, 0x00000005UL, 0x0000009aUL,
    0x00000007UL, 0x00000012UL, 0x00000080UL, 0x000000e2UL, 0x000000ebUL, 0x00000027UL, 0x000000b2UL, 0x00000075UL,
    0x00000009UL, 0x00000083UL, 0x0000002cUL, 0x0000001aUL, 0x0000001bUL, 0x0000006eUL, 0x0000005aUL, 0x000000a0UL,
    0x00000052UL, 0x0000003bUL, 0x000000d6UL, 0x000000b3UL, 0x00000029UL, 0x000000e3UL, 0x0000002fUL, 0x00000084UL,
    0x00000053UL, 0x000000d1UL, 0x00000000UL, 0x000000edUL, 0x00000020UL, 0x000000fcUL, 0x000000b1UL, 0x0000005bUL,
    0x0000006aUL, 0x000000cbUL, 0x000000beUL, 0x00000039UL, 0x0000004aUL, 0x0000004cUL, 0x00000058UL, 0x000000cfUL,
    0x000000d0UL, 0x000000efUL, 0x000000aaUL, 0x000000fbUL, 0x00000043UL, 0x0000004dUL, 0x00000033UL, 0x00000085UL,
    0x00000045UL, 0x000000f9UL, 0x00000002UL, 0x0000007fUL, 0x00000050UL, 0x0000003cUL, 0x0000009fUL, 0x000000a8UL,
    0x00000051UL, 0x000000a3UL, 0x00000040UL, 0x0000008fUL, 0x00000092UL, 0x0000009dUL, 0x00000038UL, 0x000000f5UL,
    0x000000bcUL, 0x000000b6UL, 0x000000daUL, 0x00000021UL, 0x00000010UL, 0x000000ffUL, 0x000000f3UL, 0x000000d2UL,
    0x000000cdUL, 0x0000000cUL, 0x00000013UL, 0x000000ecUL, 0x0000005fUL, 0x00000097UL, 0x00000044UL, 0x00000017UL,
    0x000000c4UL, 0x000000a7UL, 0x0000007eUL, 0x0000003dUL, 0x00000064UL, 0x0000005dUL, 0x00000019UL, 0x00000073UL,
    0x00000060UL, 0x00000081UL, 0x0000004fUL, 0x000000dcUL, 0x00000022UL, 0x0000002aUL, 0x00000090UL, 0x00000088UL,
    0x00000046UL, 0x000000eeUL, 0x000000b8UL, 0x00000014UL, 0x000000deUL, 0x0000005eUL, 0x0000000bUL, 0x000000dbUL,
    0x000000e0UL, 0x00000032UL, 0x0000003aUL, 0x0000000aUL, 0x00000049UL, 0x00000006UL, 0x00000024UL, 0x0000005cUL,
    0x000000c2UL, 0x000000d3UL, 0x000000acUL, 0x00000062UL, 0x00000091UL, 0x00000095UL, 0x000000e4UL, 0x00000079UL,
    0x000000e7UL, 0x000000c8UL, 0x00000037UL, 0x0000006dUL, 0x0000008dUL, 0x000000d5UL, 0x0000004eUL, 0x000000a9UL,
    0x0000006cUL, 0x00000056UL, 0x000000f4UL, 0x000000eaUL, 0x00000065UL, 0x0000007aUL, 0x000000aeUL, 0x00000008UL,
    0x000000baUL, 0x00000078UL, 0x00000025UL, 0x0000002eUL, 0x0000001cUL, 0x000000a6UL, 0x000000b4UL, 0x000000c6UL,
    0x000000e8UL, 0x000000ddUL, 0x00000074UL, 0x0000001fUL, 0x0000004bUL, 0x000000bdUL, 0x0000008bUL, 0x0000008aUL,
    0x00000070UL, 0x0000003eUL, 0x000000b5UL, 0x00000066UL, 0x00000048UL, 0x00000003UL, 0x000000f6UL, 0x0000000eUL,
    0x00000061UL, 0x00000035UL, 0x00000057UL, 0x000000b9UL, 0x00000086UL, 0x000000c1UL, 0x0000001dUL, 0x0000009eUL,
    0x000000e1UL, 0x000000f8UL, 0x00000098UL, 0x00000011UL, 0x00000069UL, 0x000000d9UL, 0x0000008eUL, 0x00000094UL,
    0x0000009bUL, 0x0000001eUL, 0x00000087UL, 0x000000e9UL, 0x000000ceUL, 0x00000055UL, 0x00000028UL, 0x000000dfUL,
    0x0000008cUL, 0x000000a1UL, 0x00000089UL, 0x0000000dUL, 0x000000bfUL, 0x000000e6UL, 0x00000042UL, 0x00000068UL,
    0x00000041UL, 0x00000099UL, 0x0000002dUL, 0x0000000fUL, 0x000000b0UL, 0x00000054UL, 0x000000bbUL, 0x00000016UL};

static const uint32_t AES_PRECOMP_TE4_1[256] = {
    0x00006300UL, 0x00007c00UL, 0x00007700UL, 0x00007b00UL, 0x0000f200UL, 0x00006b00UL, 0x00006f00UL, 0x0000c500UL,
    0x00003000UL, 0x00000100UL, 0x00006700UL, 0x00002b00UL, 0x0000fe00UL, 0x0000d700UL, 0x0000ab00UL, 0x00007600UL,
    0x0000ca00UL, 0x00008200UL, 0x0000c900UL, 0x00007d00UL, 0x0000fa00UL, 0x00005900UL, 0x00004700UL, 0x0000f000UL,
    0x0000ad00UL, 0x0000d400UL, 0x0000a200UL, 0x0000af00UL, 0x00009c00UL, 0x0000a400UL, 0x00007200UL, 0x0000c000UL,
    0x0000b700UL, 0x0000fd00UL, 0x00009300UL, 0x00002600UL, 0x00003600UL, 0x00003f00UL, 0x0000f700UL, 0x0000cc00UL,
    0x00003400UL, 0x0000a500UL, 0x0000e500UL, 0x0000f100UL, 0x00007100UL, 0x0000d800UL, 0x00003100UL, 0x00001500UL,
    0x00000400UL, 0x0000c700UL, 0x00002300UL, 0x0000c300UL, 0x00001800UL, 0x00009600UL, 0x00000500UL, 0x00009a00UL,
    0x00000700UL, 0x00001200UL, 0x00008000UL, 0x0000e200UL, 0x0000eb00UL, 0x00002700UL, 0x0000b200UL, 0x00007500UL,
    0x00000900UL, 0x00008300UL, 0x00002c00UL, 0x00001a00UL, 0x00001b00UL, 0x00006e00UL, 0x00005a00UL, 0x0000a000UL,
    0x00005200UL, 0x00003b00UL, 0x0000d600UL, 0x0000b300UL, 0x00002900UL, 0x0000e300UL, 0x00002f00UL, 0x00008400UL,
    0x00005300UL, 0x0000d100UL, 0x00000000UL, 0x0000ed00UL, 0x00002000UL, 0x0000fc00UL, 0x0000b100UL, 0x00005b00UL,
    0x00006a00UL, 0x0000cb00UL, 0x0000be00UL, 0x00003900UL, 0x00004a00UL, 0x00004c00UL, 0x00005800UL, 0x0000cf00UL,
    0x0000d000UL, 0x0000ef00UL, 0x0000aa00UL, 0x0000fb00UL, 0x00004300UL, 0x00004d00UL, 0x00003300UL, 0x00008500UL,
    0x00004500UL, 0x0000f900UL, 0x00000200UL, 0x00007f00UL, 0x00005000UL, 0x00003c00UL, 0x00009f00UL, 0x0000a800UL,
    0x00005100UL, 0x0000a300UL, 0x00004000UL, 0x00008f00UL, 0x00009200UL, 0x00009d00UL, 0x00003800UL, 0x0000f500UL,
    0x0000bc00UL, 0x0000b600UL, 0x0000da00UL, 0x00002100UL, 0x00001000UL, 0x0000ff00UL, 0x0000f300UL, 0x0000d200UL,
    0x0000cd00UL, 0x00000c00UL, 0x00001300UL, 0x0000ec00UL, 0x00005f00UL, 0x00009700UL, 0x00004400UL, 0x00001700UL,
    0x0000c400UL, 0x0000a700UL, 0x00007e00UL, 0x00003d00UL, 0x00006400UL, 0x00005d00UL, 0x00001900UL, 0x00007300UL,
    0x00006000UL, 0x00008100UL, 0x00004f00UL, 0x0000dc00UL, 0x00002200UL, 0x00002a00UL, 0x00009000UL, 0x00008800UL,
    0x00004600UL, 0x0000ee00UL, 0x0000b800UL, 0x00001400UL, 0x0000de00UL, 0x00005e00UL, 0x00000b00UL, 0x0000db00UL,
    0x0000e000UL, 0x00003200UL, 0x00003a00UL, 0x00000a00UL, 0x00004900UL, 0x00000600UL, 0x00002400UL, 0x00005c00UL,
    0x0000c200UL, 0x0000d300UL, 0x0000ac00UL, 0x00006200UL, 0x00009100UL, 0x00009500UL, 0x0000e400UL, 0x00007900UL,
    0x0000e700UL, 0x0000c800UL, 0x00003700UL, 0x00006d00UL, 0x00008d00UL, 0x0000d500UL, 0x00004e00UL, 0x0000a900UL,
    0x00006c00UL, 0x00005600UL, 0x0000f400UL, 0x0000ea00UL, 0x00006500UL, 0x00007a00UL, 0x0000ae00UL, 0x00000800UL,
    0x0000ba00UL, 0x00007800UL, 0x00002500UL, 0x00002e00UL, 0x00001c00UL, 0x0000a600UL, 0x0000b400UL, 0x0000c600UL,
    0x0000e800UL, 0x0000dd00UL, 0x00007400UL, 0x00001f00UL, 0x00004b00UL, 0x0000bd00UL, 0x00008b00UL, 0x00008a00UL,
    0x00007000UL, 0x00003e00UL, 0x0000b500UL, 0x00006600UL, 0x00004800UL, 0x00000300UL, 0x0000f600UL, 0x00000e00UL,
    0x00006100UL, 0x00003500UL, 0x00005700UL, 0x0000b900UL, 0x00008600UL, 0x0000c100UL, 0x00001d00UL, 0x00009e00UL,
    0x0000e100UL, 0x0000f800UL, 0x00009800UL, 0x00001100UL, 0x00006900UL, 0x0000d900UL, 0x00008e00UL, 0x00009400UL,
    0x00009b00UL, 0x00001e00UL, 0x00008700UL, 0x0000e900UL, 0x0000ce00UL, 0x00005500UL, 0x00002800UL, 0x0000df00UL,
    0x00008c00UL, 0x0000a100UL, 0x00008900UL, 0x00000d00UL, 0x0000bf00UL, 0x0000e600UL, 0x00004200UL, 0x00006800UL,
    0x00004100UL, 0x00009900UL, 0x00002d00UL, 0x00000f00UL, 0x0000b000UL, 0x00005400UL, 0x0000bb00UL, 0x00001600UL};

static const uint32_t AES_PRECOMP_TE4_2[256] = {
    0x00630000UL, 0x007c0000UL, 0x00770000UL, 0x007b0000UL, 0x00f20000UL, 0x006b0000UL, 0x006f0000UL, 0x00c50000UL,
    0x00300000UL, 0x00010000UL, 0x00670000UL, 0x002b0000UL, 0x00fe0000UL, 0x00d70000UL, 0x00ab0000UL, 0x00760000UL,
    0x00ca0000UL, 0x00820000UL, 0x00c90000UL, 0x007d0000UL, 0x00fa0000UL, 0x00590000UL, 0x00470000UL, 0x00f00000UL,
    0x00ad0000UL, 0x00d40000UL, 0x00a20000UL, 0x00af0000UL, 0x009c0000UL, 0x00a40000UL, 0x00720000UL, 0x00c00000UL,
    0x00b70000UL, 0x00fd0000UL, 0x00930000UL, 0x00260000UL, 0x00360000UL, 0x003f0000UL, 0x00f70000UL, 0x00cc0000UL,
    0x00340000UL, 0x00a50000UL, 0x00e50000UL, 0x00f10000UL, 0x00710000UL, 0x00d80000UL, 0x00310000UL, 0x00150000UL,
    0x00040000UL, 0x00c70000UL, 0x00230000UL, 0x00c30000UL, 0x00180000UL, 0x00960000UL, 0x00050000UL, 0x009a0000UL,
    0x00070000UL, 0x00120000UL, 0x00800000UL, 0x00e20000UL, 0x00eb0000UL, 0x00270000UL, 0x00b20000UL, 0x00750000UL,
    0x00090000UL, 0x00830000UL, 0x002c0000UL, 0x001a0000UL, 0x001b0000UL, 0x006e0000UL, 0x005a0000UL, 0x00a00000UL,
    0x00520000UL, 0x003b0000UL, 0x00d60000UL, 0x00b30000UL, 0x00290000UL, 0x00e30000UL, 0x002f0000UL, 0x00840000UL,
    0x00530000UL, 0x00d10000UL, 0x00000000UL, 0x00ed0000UL, 0x00200000UL, 0x00fc0000UL, 0x00b10000UL, 0x005b0000UL,
    0x006a0000UL, 0x00cb0000UL, 0x00be0000UL, 0x00390000UL, 0x004a0000UL, 0x004c0000UL, 0x00580000UL, 0x00cf0000UL,
    0x00d00000UL, 0x00ef0000UL, 0x00aa0000UL, 0x00fb0000UL, 0x00430000UL, 0x004d0000UL, 0x00330000UL, 0x00850000UL,
    0x00450000UL, 0x00f90000UL, 0x00020000UL, 0x007f0000UL, 0x00500000UL, 0x003c0000UL, 0x009f0000UL, 0x00a80000UL,
    0x00510000UL, 0x00a30000UL, 0x00400000UL, 0x008f0000UL, 0x00920000UL, 0x009d0000UL, 0x00380000UL, 0x00f50000UL,
    0x00bc0000UL, 0x00b60000UL, 0x00da0000UL, 0x00210000UL, 0x00100000UL, 0x00ff0000UL, 0x00f30000UL, 0x00d20000UL,
    0x00cd0000UL, 0x000c0000UL, 0x00130000UL, 0x00ec0000UL, 0x005f0000UL, 0x00970000UL, 0x00440000UL, 0x00170000UL,
    0x00c40000UL, 0x00a70000UL, 0x007e0000UL, 0x003d0000UL, 0x00640000UL, 0x005d0000UL, 0x00190000UL, 0x00730000UL,
    0x00600000UL, 0x00810000UL, 0x004f0000UL, 0x00dc0000UL, 0x00220000UL, 0x002a0000UL, 0x00900000UL, 0x00880000UL,
    0x00460000UL, 0x00ee0000UL, 0x00b80000UL, 0x00140000UL, 0x00de0000UL, 0x005e0000UL, 0x000b0000UL, 0x00db0000UL,
    0x00e00000UL, 0x00320000UL, 0x003a0000UL, 0x000a0000UL, 0x00490000UL, 0x00060000UL, 0x00240000UL, 0x005c0000UL,
    0x00c20000UL, 0x00d30000UL, 0x00ac0000UL, 0x00620000UL, 0x00910000UL, 0x00950000UL, 0x00e40000UL, 0x00790000UL,
    0x00e70000UL, 0x00c80000UL, 0x00370000UL, 0x006d0000UL, 0x008d0000UL, 0x00d50000UL, 0x004e0000UL, 0x00a90000UL,
    0x006c0000UL, 0x00560000UL, 0x00f40000UL, 0x00ea0000UL, 0x00650000UL, 0x007a0000UL, 0x00ae0000UL, 0x00080000UL,
    0x00ba0000UL, 0x00780000UL, 0x00250000UL, 0x002e0000UL, 0x001c0000UL, 0x00a60000UL, 0x00b40000UL, 0x00c60000UL,
    0x00e80000UL, 0x00dd0000UL, 0x00740000UL, 0x001f0000UL, 0x004b0000UL, 0x00bd0000UL, 0x008b0000UL, 0x008a0000UL,
    0x00700000UL, 0x003e0000UL, 0x00b50000UL, 0x00660000UL, 0x00480000UL, 0x00030000UL, 0x00f60000UL, 0x000e0000UL,
    0x00610000UL, 0x00350000UL, 0x00570000UL, 0x00b90000UL, 0x00860000UL, 0x00c10000UL, 0x001d0000UL, 0x009e0000UL,
    0x00e10000UL, 0x00f80000UL, 0x00980000UL, 0x00110000UL, 0x00690000UL, 0x00d90000UL, 0x008e0000UL, 0x00940000UL,
    0x009b0000UL, 0x001e0000UL, 0x00870000UL, 0x00e90000UL, 0x00ce0000UL, 0x00550000UL, 0x00280000UL, 0x00df0000UL,
    0x008c0000UL, 0x00a10000UL, 0x00890000UL, 0x000d0000UL, 0x00bf0000UL, 0x00e60000UL, 0x00420000UL, 0x00680000UL,
    0x00410000UL, 0x00990000UL, 0x002d0000UL, 0x000f0000UL, 0x00b00000UL, 0x00540000UL, 0x00bb0000UL, 0x00160000UL};

static const uint32_t AES_PRECOMP_TE4_3[256] = {
    0x63000000UL, 0x7c000000UL, 0x77000000UL, 0x7b000000UL, 0xf2000000UL, 0x6b000000UL, 0x6f000000UL, 0xc5000000UL,
    0x30000000UL, 0x01000000UL, 0x67000000UL, 0x2b000000UL, 0xfe000000UL, 0xd7000000UL, 0xab000000UL, 0x76000000UL,
    0xca000000UL, 0x82000000UL, 0xc9000000UL, 0x7d000000UL, 0xfa000000UL, 0x59000000UL, 0x47000000UL, 0xf0000000UL,
    0xad000000UL, 0xd4000000UL, 0xa2000000UL, 0xaf000000UL, 0x9c000000UL, 0xa4000000UL, 0x72000000UL, 0xc0000000UL,
    0xb7000000UL, 0xfd000000UL, 0x93000000UL, 0x26000000UL, 0x36000000UL, 0x3f000000UL, 0xf7000000UL, 0xcc000000UL,
    0x34000000UL, 0xa5000000UL, 0xe5000000UL, 0xf1000000UL, 0x71000000UL, 0xd8000000UL, 0x31000000UL, 0x15000000UL,
    0x04000000UL, 0xc7000000UL, 0x23000000UL, 0xc3000000UL, 0x18000000UL, 0x96000000UL, 0x05000000UL, 0x9a000000UL,
    0x07000000UL, 0x12000000UL, 0x80000000UL, 0xe2000000UL, 0xeb000000UL, 0x27000000UL, 0xb2000000UL, 0x75000000UL,
    0x09000000UL, 0x83000000UL, 0x2c000000UL, 0x1a000000UL, 0x1b000000UL, 0x6e000000UL, 0x5a000000UL, 0xa0000000UL,
    0x52000000UL, 0x3b000000UL, 0xd6000000UL, 0xb3000000UL, 0x29000000UL, 0xe3000000UL, 0x2f000000UL, 0x84000000UL,
    0x53000000UL, 0xd1000000UL, 0x00000000UL, 0xed000000UL, 0x20000000UL, 0xfc000000UL, 0xb1000000UL, 0x5b000000UL,
    0x6a000000UL, 0xcb000000UL, 0xbe000000UL, 0x39000000UL, 0x4a000000UL, 0x4c000000UL, 0x58000000UL, 0xcf000000UL,
    0xd0000000UL, 0xef000000UL, 0xaa000000UL, 0xfb000000UL, 0x43000000UL, 0x4d000000UL, 0x33000000UL, 0x85000000UL,
    0x45000000UL, 0xf9000000UL, 0x02000000UL, 0x7f000000UL, 0x50000000UL, 0x3c000000UL, 0x9f000000UL, 0xa8000000UL,
    0x51000000UL, 0xa3000000UL, 0x40000000UL, 0x8f000000UL, 0x92000000UL, 0x9d000000UL, 0x38000000UL, 0xf5000000UL,
    0xbc000000UL, 0xb6000000UL, 0xda000000UL, 0x21000000UL, 0x10000000UL, 0xff000000UL, 0xf3000000UL, 0xd2000000UL,
    0xcd000000UL, 0x0c000000UL, 0x13000000UL, 0xec000000UL, 0x5f000000UL, 0x97000000UL, 0x44000000UL, 0x17000000UL,
    0xc4000000UL, 0xa7000000UL, 0x7e000000UL, 0x3d000000UL, 0x64000000UL, 0x5d000000UL, 0x19000000UL, 0x73000000UL,
    0x60000000UL, 0x81000000UL, 0x4f000000UL, 0xdc000000UL, 0x22000000UL, 0x2a000000UL, 0x90000000UL, 0x88000000UL,
    0x46000000UL, 0xee000000UL, 0xb8000000UL, 0x14000000UL, 0xde000000UL, 0x5e000000UL, 0x0b000000UL, 0xdb000000UL,
    0xe0000000UL, 0x32000000UL, 0x3a000000UL, 0x0a000000UL, 0x49000000UL, 0x06000000UL, 0x24000000UL, 0x5c000000UL,
    0xc2000000UL, 0xd3000000UL, 0xac000000UL, 0x62000000UL, 0x91000000UL, 0x95000000UL, 0xe4000000UL, 0x79000000UL,
    0xe7000000UL, 0xc8000000UL, 0x37000000UL, 0x6d000000UL, 0x8d000000UL, 0xd5000000UL, 0x4e000000UL, 0xa9000000UL,
    0x6c000000UL, 0x56000000UL, 0xf4000000UL, 0xea000000UL, 0x65000000UL, 0x7a000000UL, 0xae000000UL, 0x08000000UL,
    0xba000000UL, 0x78000000UL, 0x25000000UL, 0x2e000000UL, 0x1c000000UL, 0xa6000000UL, 0xb4000000UL, 0xc6000000UL,
    0xe8000000UL, 0xdd000000UL, 0x74000000UL, 0x1f000000UL, 0x4b000000UL, 0xbd000000UL, 0x8b000000UL, 0x8a000000UL,
    0x70000000UL, 0x3e000000UL, 0xb5000000UL, 0x66000000UL, 0x48000000UL, 0x03000000UL, 0xf6000000UL, 0x0e000000UL,
    0x61000000UL, 0x35000000UL, 0x57000000UL, 0xb9000000UL, 0x86000000UL, 0xc1000000UL, 0x1d000000UL, 0x9e000000UL,
    0xe1000000UL, 0xf8000000UL, 0x98000000UL, 0x11000000UL, 0x69000000UL, 0xd9000000UL, 0x8e000000UL, 0x94000000UL,
    0x9b000000UL, 0x1e000000UL, 0x87000000UL, 0xe9000000UL, 0xce000000UL, 0x55000000UL, 0x28000000UL, 0xdf000000UL,
    0x8c000000UL, 0xa1000000UL, 0x89000000UL, 0x0d000000UL, 0xbf000000UL, 0xe6000000UL, 0x42000000UL, 0x68000000UL,
    0x41000000UL, 0x99000000UL, 0x2d000000UL, 0x0f000000UL, 0xb0000000UL, 0x54000000UL, 0xbb000000UL, 0x16000000UL};

static const uint32_t masks[32] = {
    0x80000000UL, 0x40000000UL, 0x20000000UL, 0x10000000UL, 0x08000000UL, 0x04000000UL, 0x02000000UL, 0x01000000UL,
    0x00800000UL, 0x00400000UL, 0x00200000UL, 0x00100000UL, 0x00080000UL, 0x00040000UL, 0x00020000UL, 0x00010000UL,
    0x00008000UL, 0x00004000UL, 0x00002000UL, 0x00001000UL, 0x00000800UL, 0x00000400UL, 0x00000200UL, 0x00000100UL,
    0x00000080UL, 0x00000040UL, 0x00000020UL, 0x00000010UL, 0x00000008UL, 0x00000004UL, 0x00000002UL, 0x00000001UL};

CryptoUtils::CryptoUtils() : gen(0x1) { seeded = false; }

unsigned CryptoUtils::scramble32(const unsigned in, const char key[16]) {
    assert(key != NULL && "CryptoUtils::scramble key=NULL");

    // fixme:
    //DEBUG_WITH_TYPE("cryptoutils", dbgs() << "scramble32 in with " << in << "\n");

    unsigned tmpA, tmpB;

    // Orr, Nathan or Adi can probably break it, but who cares?

    // Round 1
    tmpA = 0x0;
    tmpA ^= AES_PRECOMP_TE0[((in >> 24) ^ key[0]) & 0xFF];
    tmpA ^= AES_PRECOMP_TE1[((in >> 16) ^ key[1]) & 0xFF];
    tmpA ^= AES_PRECOMP_TE2[((in >> 8) ^ key[2]) & 0xFF];
    tmpA ^= AES_PRECOMP_TE3[((in >> 0) ^ key[3]) & 0xFF];

    // Round 2
    tmpB = 0x0;
    tmpB ^= AES_PRECOMP_TE0[((tmpA >> 24) ^ key[4]) & 0xFF];
    tmpB ^= AES_PRECOMP_TE1[((tmpA >> 16) ^ key[5]) & 0xFF];
    tmpB ^= AES_PRECOMP_TE2[((tmpA >> 8) ^ key[6]) & 0xFF];
    tmpB ^= AES_PRECOMP_TE3[((tmpA >> 0) ^ key[7]) & 0xFF];

    // Round 3
    tmpA = 0x0;
    tmpA ^= AES_PRECOMP_TE0[((tmpB >> 24) ^ key[8]) & 0xFF];
    tmpA ^= AES_PRECOMP_TE1[((tmpB >> 16) ^ key[9]) & 0xFF];
    tmpA ^= AES_PRECOMP_TE2[((tmpB >> 8) ^ key[10]) & 0xFF];
    tmpA ^= AES_PRECOMP_TE3[((tmpB >> 0) ^ key[11]) & 0xFF];

    // Round 4
    tmpB = 0x0;
    tmpB ^= AES_PRECOMP_TE0[((tmpA >> 24) ^ key[12]) & 0xFF];
    tmpB ^= AES_PRECOMP_TE1[((tmpA >> 16) ^ key[13]) & 0xFF];
    tmpB ^= AES_PRECOMP_TE2[((tmpA >> 8) ^ key[14]) & 0xFF];
    tmpB ^= AES_PRECOMP_TE3[((tmpA >> 0) ^ key[15]) & 0xFF];

    LOAD32H(tmpA, key);



    //DEBUG_WITH_TYPE("cryptoutils", dbgs() << "scramble32 out with " << static_cast<unsigned>(tmpA ^ tmpB) << "\n");
    return tmpA ^ tmpB;
}

void CryptoUtils::prng_seed(const std::string _seed) {
    unsigned char s[4];
    unsigned int i = 0;

    /* We accept a prefix "0x" */
    if (!(_seed.size() == 10 || _seed.size() == 8)) {
        LLVMContext ctx;
        ctx.emitError(Twine("The  seeding mechanism is expecting a 4-byte value "
                            "expressed in hexadecimal, like DEADBEEF"));
    }

    seed = _seed;

    if (_seed.size() == 10) {
        // Assuming that the two first characters are "0x"
        i = 2;
    }

    for (; i < _seed.length(); i += 2) {
        std::string byte = _seed.substr(i, 2);
        s[i >> 1] = (unsigned char)(int)strtol(byte.c_str(), NULL, 16);
    }

    // _seed is defined to be the
    // key initial value
    memcpy(key, s, 4);
    unsigned seeds = *((unsigned *)key);
    // Once the seed is there, we compute the
    // AES128 key-schedule
    std::mt19937 gens(seeds);
    this->gen = gens;
    seeded = true;
    // Fill the pool with psuedo-random values
    populate_pool();
}

CryptoUtils::~CryptoUtils() {
    // Some wiping work here
    memset(key, 0, 4);
    memset(pool, 0, CryptoUtils_POOL_SIZE);
    idx = 0;
}

void CryptoUtils::prng_seed() {
    std::random_device rd;
    std::mt19937 gens(rd());
    this->gen = gens;
    seeded = true;
}

void CryptoUtils::populate_pool() {
    for (int i = 0; i < CryptoUtils_POOL_SIZE; i += 4) {
        unsigned r = gen();
        memcpy(pool + i, &r, 4);
    }
    idx = 0;
}

void CryptoUtils::get_bytes(char *buffer, const int len) {

    int sofar = 0, available = 0;

    assert(buffer != NULL && "CryptoUtils::get_bytes buffer=NULL");
    assert(len > 0 && "CryptoUtils::get_bytes len <= 0");

    if (len > 0) {

        // If the PRNG is not seeded, it the very last time to do it !
        if (!seeded) {
            prng_seed();
            populate_pool();
        }

        do {
            if (idx + (len - sofar) >= CryptoUtils_POOL_SIZE) {
                // We don't have enough bytes ready in the pool,
                // so let's use the available ones and repopulate !
                available = CryptoUtils_POOL_SIZE - idx;
                memcpy(buffer + sofar, pool + idx, available);
                sofar += available;
                populate_pool();
            } else {
                memcpy(buffer + sofar, pool + idx, len - sofar);
                idx += len - sofar;
                // This will trigger a loop exit
                sofar = len;
            }
        } while (sofar < (len - 1));
    }
}

uint8_t CryptoUtils::get_uint8_t() {
    char ret;

    get_bytes(&ret, 1);

    return (uint8_t)ret;
}

char CryptoUtils::get_char() {
    char ret;

    get_bytes(&ret, 1);

    return ret;
}

uint32_t CryptoUtils::get_uint32_t() {
    char tmp[4];
    uint32_t ret = 0;

    get_bytes(tmp, 4);

    LOAD32H(ret, tmp);

    return ret;
}

uint64_t CryptoUtils::get_uint64_t() {
    char tmp[8];
    uint64_t ret = 0;

    get_bytes(tmp, 8);

    LOAD64H(ret, tmp);

    return ret;
}

uint32_t CryptoUtils::get_range(const uint32_t max) {
    uint32_t log, r, mask;

    if (max == 0) {
        return 0;
    } else {
        // Computing the above power of two
        log = 32;
        int i = 0;
        // This loop will terminate, as there is at least one
        // bit set somewhere in max
        while (!(max & masks[i++])) {
            log -= 1;
        }
        mask = (0x1UL << log) - 1;

        // This should loop two times in average
        do {
            r = get_uint32_t() & mask;
        } while (r >= max);

        return r;
    }
}