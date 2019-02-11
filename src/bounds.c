#include <termlike/bounds.h> // term_bounds, term_align

struct term_bounds const TERM_BOUNDS_NONE = {
    .align = TERM_ALIGN_LEFT,
    .wrap = TERM_WRAP_CHARACTERS,
    .size = {
        .width = TERM_BOUNDS_UNBOUNDED,
        .height = TERM_BOUNDS_UNBOUNDED
    },
    .limit = TERM_BOUNDS_UNBOUNDED
};

extern inline struct term_dimens sized(int32_t width, int32_t height);
extern inline struct term_bounds boxed(int32_t width, int32_t height,
                                       enum term_align);
extern inline struct term_bounds bounded(int32_t width, int32_t height);
extern inline struct term_bounds aligned(enum term_align);
