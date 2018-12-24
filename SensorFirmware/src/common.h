#pragma once

#define IF_ERR(s) if ((err = (s)) < 0)

#define RET_ERR(s) IF_ERR(s) return err