// mod.description = 'Ensures only one track can be armed at any one time'
// mod.category = 'global'

host.showPopupNotification('Exclusive arm successfully loaded')
// tracks.forEach((t, i) => {
//     t.arm().addValueObserver(armed => {
//         if (armed) {
//             // Unarm all other tracks
//             tracks.forEach(t => {
//                 if (i !== i2) {
//                     t.arm().set(false);
//                 }
//             })
//         }
//     })
// })