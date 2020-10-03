/**
 * @name Exclusive Arm
 * @id exclusive-arm
 * @description Ensures only one track can be armed at any one time
 * @category global
 */

tracks.forEach((t, i) => {
    t.arm().addValueObserver(armed => {
        if (armed) {
            // Unarm all other tracks
            tracks.forEach((t, i2) => {
                if (i !== i2) {
                    t.arm().set(false);
                }
            })
        }
    })
})