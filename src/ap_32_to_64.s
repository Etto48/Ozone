.text
.code32
.global ap_startup
ap_startup:
    hlt
    ljmp $8, $ap_long_mode_start

.code64
ap_long_mode_start:
    hlt

.data
