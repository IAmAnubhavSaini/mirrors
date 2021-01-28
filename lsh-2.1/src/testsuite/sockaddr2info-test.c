#include "testutils.h"

#include <netinet/in.h>
#include <arpa/inet.h>

static void
test_ip_v4(const char *addr, unsigned port)
{
  struct sockaddr_in sa;
  struct address_info *info;

  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = inet_addr(addr);
  sa.sin_port = htons(port);

  info = sockaddr2info(sizeof(sa), (struct sockaddr *) &sa);

  ASSERT(info);
  ASSERT(lsh_string_eq_l(info->ip, strlen(addr), addr));
  ASSERT(info->port == port);
}

#if WITH_IPV6
static void
test_ip_v6(const char *addr, unsigned port)
{
  struct sockaddr_in6 sa;
  struct address_info *info;

  sa.sin6_family = AF_INET6;
  ASSERT(inet_pton(AF_INET6, addr, &sa.sin6_addr));
  sa.sin6_port = htons(port);

  info = sockaddr2info(sizeof(sa), (struct sockaddr *) &sa);
  ASSERT(lsh_string_eq_l(info->ip, strlen(addr), addr));
  ASSERT(info->port == port);
}
#endif /* WITH_IPV6 */

int
test_main(void)
{
  test_ip_v4("127.0.0.1", 22);
  
#if WITH_IPV6
#if 0
  const uint8_t a1[16] =
    { 0x1080, 0, 0, 0, 0, 0, 0, 0, 0x8, 0x8, 0, 0x20, 0xC, 0x41, 0x7A };
  const uint8_t a2[16] =
    { 0xFF, 0x1, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0x43 };
  const uint8_t a3[16] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
  const uint8_t a3[16] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif
  test_ip_v6("1080::8:800:200c:417a", 22);
  test_ip_v6("ff01::43", 4711);
  test_ip_v6("::1", 80);
  test_ip_v6("::", 17);

  SUCCESS();
#else /* WITH_IPV6 */
  SKIP();
#endif /* WITH_IPV6 */
}
