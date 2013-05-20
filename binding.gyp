{
  'targets': [
    {
      'target_name': 'GraphicsMagick',
      'sources': [ 'GraphicsMagick.cc' ],
      'libraries': [
        '<!@(GraphicsMagick-config --libs)'
      ],
      'conditions': [
        ['OS=="mac"', {
          # cflags on OS X are stupid and have to be defined like this
          'xcode_settings': {
            'OTHER_CFLAGS': [
              '<!@(GraphicsMagick-config --cflags)',
              '<!@(GraphicsMagick-config --cppflags)'
            ]
          }
        }, {
          'cflags': [
            '<!@(GraphicsMagick-config --cflags)',
            '<!@(GraphicsMagick-config --cppflags)'
          ],
        }]
      ]
    }
  ]
}
