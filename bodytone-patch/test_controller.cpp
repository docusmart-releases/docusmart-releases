#include "../bodytone-patch/bodytone_safe_erg.h"
#include <cassert>
#include <cmath>
#include <limits>
int main() {
  bodytone::SafeErgController c;
  c.setTarget(256, 0);
  assert(c.active());
  auto cmd = c.tick(80, 100, true, 0);
  assert(cmd.send && cmd.gradePercent == 0.0);
  assert(!c.tick(80, 100, true, 4000).send);
  cmd = c.tick(80, 100, true, 8000);
  assert(cmd.send && cmd.gradePercent <= 0.25);
  c.tick(80, 100, true, 12000);
  assert(c.gradePercent() <= bodytone::SafeErgController::kMaxGrade);
  cmd = c.tick(40, 0, true, 16000);
  assert(c.gradePercent() == 0.0);
  c.cancel();
  c.setTarget(300, 20000);
  assert(c.tick(80, 100, true, 20000).send);
  assert(!c.tick(80, 100, true, 24000).send);
  c.tick(80, 100, true, 28000);
  cmd = c.tick(80, 100, true, 70000);
  assert(!c.active() && c.gradePercent() <= 5.0);
  return 0;
}
