/*******************************************************************************
*   (c) 2018, 2019 ZondaX GmbH
*   (c) 2016 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "view.h"
#include "actions.h"
#include "apdu_codes.h"
#include "glyphs.h"
#include "bagl.h"
#include "zxmacros.h"
#include "view_templates.h"
#include "transaction.h"

#include <string.h>
#include <stdio.h>

view_t viewdata;

void h_back() {
    view_idle_show(0);
    UX_WAIT();
}

void view_sign_internal_show();

int8_t view_update_review();

void view_review_show(void);

void h_review(unsigned int _) {
    UNUSED(_);
    viewdata.idx = 0;
    viewdata.pageIdx = 0;
    viewdata.pageCount = 1;
    view_update_review();
    view_review_show();
}

void h_decrease() {
    viewdata.pageIdx--;
    if (viewdata.pageIdx < 0) {
        viewdata.idx--;
        viewdata.pageIdx = 0;
    }
}

void h_increase() {
    viewdata.pageIdx++;
    if (viewdata.pageIdx >= viewdata.pageCount) {
        viewdata.idx++;
        viewdata.pageIdx = 0;
    }
}

void h_sign_accept(unsigned int _) {
    UNUSED(_);

    const uint8_t replyLen = app_sign();

    view_idle_show(0);
    UX_WAIT();

    set_code(G_io_apdu_buffer, replyLen, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, replyLen + 2);
}

void h_sign_reject(unsigned int _) {
    UNUSED(_);
    view_idle_show(0);
    UX_WAIT();

    set_code(G_io_apdu_buffer, 0, APDU_CODE_COMMAND_NOT_ALLOWED);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}

void splitValueField() {
#if defined(TARGET_NANOS)
    print_value2("");
    uint16_t vlen = strlen(viewdata.value);
    if (vlen > MAX_CHARS_PER_VALUE2_LINE - 1) {
        strcpy(viewdata.value2, viewdata.value + MAX_CHARS_PER_VALUE_LINE);
        viewdata.value[MAX_CHARS_PER_VALUE_LINE] = 0;
    }
#endif
}

#if defined(TARGET_NANOX)

#include "ux.h"
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

#ifdef TESTING_ENABLED
UX_FLOW_DEF_NOCB(ux_idle_flow_1_step, pbb, { &C_icon_app, "Matrix AI", "TEST!", });
#else
UX_FLOW_DEF_NOCB(ux_idle_flow_1_step, pbb, { &C_icon_app, "Matrix AI", "Network", });
#endif
UX_FLOW_DEF_NOCB(ux_idle_flow_3_step, bn, { "Version", APPVERSION, });
UX_FLOW_DEF_VALID(ux_idle_flow_4_step, pb, os_sched_exit(-1), { &C_icon_dashboard, "Quit",});
const ux_flow_step_t *const ux_idle_flow [] = {
  &ux_idle_flow_1_step,
  &ux_idle_flow_3_step,
  &ux_idle_flow_4_step,
  FLOW_END_STEP,
};

UX_STEP_NOCB(ux_addr_flow_1_step, bnnn_paging, { .title = viewdata.key, .text = viewdata.value, });
UX_STEP_VALID(ux_addr_flow_2_step, pb, h_back(), { &C_icon_validate_14, "Ok"});

UX_FLOW(
    ux_addr_flow,
    &ux_addr_flow_1_step,
    &ux_addr_flow_2_step
);

typedef struct
{
unsigned char inside : 1;
unsigned char no_more_data : 1;
} review_state_t;

review_state_t review_state;

void h_review_start()
{
    if (review_state.inside) {
    // coming from right
        viewdata.idx--;
        if (viewdata.idx<0) {
            // exit to the left
            review_state.inside = 0;
            ux_flow_prev();
            return;
        }
    } else {
    // coming from left
        viewdata.idx = 0;
    }

    view_update_review();
    ux_flow_next();
}

void h_review_data()
{
    review_state.inside = 1;
}

void h_review_end()
{
    if (review_state.inside) {
    // coming from left
        viewdata.idx++;
        if (view_update_review()== TX_NO_MORE_DATA){
            review_state.inside = 0;
            ux_flow_next();
            return;
        }
        ux_layout_bnnn_paging_reset();
    } else {
    // coming from right
        viewdata.idx--;
        view_update_review();
    }

    // move to prev flow but trick paging to show first page
    CUR_FLOW.prev_index = CUR_FLOW.index-2;
    CUR_FLOW.index--;
    ux_flow_relayout();
}

UX_STEP_NOCB(ux_sign_flow_1_step, pbb, { &C_icon_eye, "Review", "Transaction" });

UX_STEP_INIT(ux_sign_flow_2_start_step, NULL, NULL, { h_review_start(); });
UX_STEP_NOCB_INIT(ux_sign_flow_2_step, bnnn_paging, { h_review_data(); }, { .title = viewdata.key, .text = viewdata.value, });
UX_STEP_INIT(ux_sign_flow_2_end_step, NULL, NULL, { h_review_end(); });

UX_STEP_VALID(ux_sign_flow_3_step, pbb, h_sign_accept(0), { &C_icon_validate_14, "Sign", "Transaction" });
UX_STEP_VALID(ux_sign_flow_4_step, pbb, h_sign_reject(0), { &C_icon_crossmark, "Reject", "Transaction" });
const ux_flow_step_t *const ux_sign_flow[] = {
  &ux_sign_flow_1_step,
  &ux_sign_flow_2_start_step,
  &ux_sign_flow_2_step,
  &ux_sign_flow_2_end_step,
  &ux_sign_flow_3_step,
  &ux_sign_flow_4_step,
  FLOW_END_STEP,
};

#else

// Nano S
ux_state_t ux;

const ux_menu_entry_t menu_main[] = {
#ifdef TESTING_ENABLED
    {NULL, NULL, 0, &C_icon_app, "Matrix AI", "TEST!", 33, 12},
#else
    {NULL, NULL, 0, &C_icon_app, "Matrix AI", "Network", 33, 12},
#endif
    {NULL, NULL, 0, NULL, "v"APPVERSION, NULL, 0, 0},
    {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
    UX_MENU_END
};

static const bagl_element_t view_address[] = {
    UI_FillRectangle(0, 0, 0, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT, 0x000000, 0xFFFFFF),
    UI_Icon(0, 128 - 7, 0, 7, 7, BAGL_GLYPH_ICON_CHECK),
    UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.key),
    UI_LabelLine(UIID_LABEL + 0, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value),
    UI_LabelLineScrolling(UIID_LABELSCROLL, 14, 30, 100, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value2),
};

const ux_menu_entry_t menu_sign[] = {
    {NULL, h_review, 0, NULL, "View transaction", NULL, 0, 0},
    {NULL, h_sign_accept, 0, NULL, "Sign transaction", NULL, 0, 0},
    {NULL, h_sign_reject, 0, &C_icon_back, "Reject", NULL, 60, 40},
    UX_MENU_END
};

static const bagl_element_t view_review[] = {
    UI_BACKGROUND_LEFT_RIGHT_ICONS,
    UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.key),
    UI_LabelLine(UIID_LABEL + 1, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value),
    UI_LabelLine(UIID_LABEL + 2, 0, 30, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value2),
};

static unsigned int view_address_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            view_idle_show(0);
            UX_WAIT();
            app_reply_address();
            break;
    }
    return 0;
}

static unsigned int view_review_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            // Press both left and right buttons to quit
            view_sign_internal_show();
            break;
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            // Press left to progress to the previous element
            h_decrease();
            if (view_update_review() == TX_NO_MORE_DATA) {
                view_sign_internal_show();
            } else {
                view_review_show();
            }
            UX_WAIT();
            break;

        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            // Press right to progress to the next element
            h_increase();
            if (view_update_review() == TX_NO_MORE_DATA) {
                view_sign_internal_show();
            } else {
                view_review_show();
            }
            UX_WAIT();
            break;
    }
    return 0;
}

const bagl_element_t *view_prepro(const bagl_element_t *element) {
    switch (element->component.userid) {
        case UIID_ICONLEFT:
        case UIID_ICONRIGHT:
            UX_CALLBACK_SET_INTERVAL(2000);
            break;
        case UIID_LABELSCROLL:
            UX_CALLBACK_SET_INTERVAL(
                MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7))
            );
            break;
    }
    return element;
}

#endif

////////////////////////////////
////////////////////////////////
////////////////////////////////

void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}

void view_init(void) {
    UX_INIT();
}

void view_idle_show(unsigned int ignored) {
#if defined(TARGET_NANOS)
    UX_MENU_DISPLAY(0, menu_main, NULL);
#elif defined(TARGET_NANOX)
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
#endif
}

void view_address_show() {
    // Address has been placed in the output buffer
    char *const manAddress = (char *) (G_io_apdu_buffer + 65);
    snprintf(viewdata.key, MAX_CHARS_PER_KEY_LINE, "Confirm address");
    snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, "%s", manAddress);
    splitValueField();
#if defined(TARGET_NANOS)
    UX_DISPLAY(view_address, view_prepro);
#elif defined(TARGET_NANOX)
    ux_layout_bnnn_paging_reset();
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_addr_flow, NULL);
#endif
}

void view_sign_show() {
#if defined(TARGET_NANOS)
    h_review(0);
#elif defined(TARGET_NANOX)
    view_sign_internal_show();
#endif
}

void view_sign_internal_show(void) {
#if defined(TARGET_NANOS)
    UX_MENU_DISPLAY(0, menu_sign, NULL);
#elif defined(TARGET_NANOX)
    viewdata.idx = -1;
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    review_state.inside = 0;
    review_state.no_more_data = 0;
    ux_flow_init(0, ux_sign_flow, NULL);
#endif
}

void view_review_show(void) {
#if defined(TARGET_NANOS)
    UX_DISPLAY(view_review, view_prepro);
#endif
}

int8_t view_update_review() {
    int8_t err = TX_NO_ERROR;

    err = transaction_getItem(viewdata.idx,
                              viewdata.key, MAX_CHARS_PER_KEY_LINE,
                              viewdata.value, MAX_CHARS_PER_VALUE1_LINE,
                              viewdata.pageIdx, &viewdata.pageCount);

    if (err == TX_NO_MORE_DATA) {
        return TX_NO_MORE_DATA;
    }

    if (err != TX_NO_ERROR) {
        print_key("");
        print_value("");
        // TODO: Reject and fail immediately
        return TX_NO_MORE_DATA;
    }

    splitValueField();

    return TX_NO_ERROR;
}
